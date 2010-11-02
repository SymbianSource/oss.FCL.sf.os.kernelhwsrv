// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_notif.cpp
// 
//

#include "sf_std.h"
#include "sf_notifier.h"
#include "sf_pool.h"

#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
#include <f32notification.h>
#endif

TChangeQue FsNotify::iChangeQues[KMaxNotifyQues];
TDiskSpaceQue FsNotify::iDiskSpaceQues[KMaxDiskQues];
TDebugQue FsNotify::iDebugQue;
TDismountNotifyQue FsNotify::iDismountNotifyQue;

_LIT(KEmptyString,"");

CFsNotificationInfoBody::CFsNotificationInfoBody()
: iSrc(KEmptyString),
  iSrcBuf(KEmptyString),
  iDest(KEmptyString),
  iDestDriveStored(EFalse),
  iFunction(KErrNotFound),
  iData(KErrNotFound), 
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
  iNotificationType(TFsNotification::EOverflow),
#else
  iNotificationType(KErrNotFound),
#endif
  iUid(TUid::Null())
    {
    }

CFsNotificationInfo::CFsNotificationInfo()
    {
    }

/*
 * These do not get deleted, they get Freed via CFsNotificationInfo::Free.
 */
CFsNotificationInfo::~CFsNotificationInfo()
    {
    Fault(ENotificationInfoDeletion);
    }

TInt CFsNotificationInfo::Init(TInt aFunction, TInt aDriveNum)
    {
    //Clean notification before use
    CleanNotification();
    
    iBody->iFunction = aFunction;
    TInt err = SetDriveNumber(aDriveNum);
    if(err != KErrNone)
        {
        return err;
        }
 
    //Set notification type
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
    CFsNotificationInfo::NotificationType(iBody->iFunction, iBody->iNotificationType);
#else
    iBody->iNotificationType = KErrNotSupported;
#endif   
    return KErrNone;
    }

void CFsNotificationInfo::CleanNotification()
    {
    //Clear all variables 
    TParsePtrC empty(KEmptyString);
    memcpy(&iBody->iSrc,&empty,sizeof(TParsePtrC));
    memcpy(&iBody->iDest,&empty,sizeof(TParsePtrC));
    iBody->iData = KErrNotFound;
    iBody->iRequest = NULL;
    iBody->iDriveNumber = KErrNotFound;
    iBody->iUid = TUid::Null();
    iBody->iDestDriveStored = EFalse;
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
    iBody->iNotificationType=TFsNotification::EOverflow;
#else
    iBody->iNotificationType=KErrNotFound;
#endif
    }

EXPORT_C CFsNotificationInfo* CFsNotificationInfo::Allocate(const CMountCB& aMount, TInt aFunction)
    {
    CFsNotificationInfo* info = NotificationInfoPool->Allocate();
    __ASSERT_DEBUG(info,User::Panic(_L("CFsNotificationInfo::Allocate, Could not allocate"),KErrNotFound));
    
    TInt driveNum = aMount.Drive().DriveNumber();
    __ASSERT_ALWAYS((driveNum >= EDriveA && driveNum <= EDriveZ), User::Panic(_L("CFsNotificationInfo::Allocate - Invalid Drive Num"),KErrArgument));
    TInt err = info->Init(aFunction,driveNum);
    if(err != KErrNone)
        {
        Free(info);
        return NULL;
        }
    return info;
    }


CFsNotificationInfo* CFsNotificationInfo::Allocate(CFsMessageRequest& aRequest)
    {
    //Get a notification Info block from the pool.
    CFsNotificationInfo* notificationInfo = NotificationInfoPool->Allocate();
    
    //Set the function and call Init.
    TInt function = aRequest.Operation()->Function();
    notificationInfo->iBody->iFunction = function;
    TInt err = notificationInfo->Init(function,aRequest.DriveNumber());
    if(err != KErrNone)
        {
        CFsNotificationInfo::Free(notificationInfo);
        return NULL;
        }

    //Set request
    notificationInfo->SetRequest(&aRequest);

	//Set UID
    notificationInfo->SetUid(aRequest.Uid()); 

    //Set notification type
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION
    CFsNotificationInfo::SetData(&aRequest,notificationInfo);
#endif    
    
    CFsClientMessageRequest& msgRequest = (CFsClientMessageRequest&)aRequest;
    
    //Get and store Src
    CFsNotificationInfo::PathName(msgRequest,notificationInfo->Source());
    
	//Get and store NewName/Dest
    switch(function)
        {
        case EFsFileRename:
        case EFsRename:    
        case EFsReplace:   
        case EFsSetDriveName:
        case EFsSetVolume:
            {
            CFsNotificationInfo::NewPathName((CFsClientMessageRequest&)aRequest,notificationInfo->NewName());
            notificationInfo->iBody->iDestDriveStored = ETrue;
            }
        default:
            break;
        }
    return notificationInfo;
    }

CFsNotificationInfo* CFsNotificationInfo::Allocate(TInt aFunction, TInt aDrive)
    {
    //Get a notification Info block from the pool.
    CFsNotificationInfo* notificationInfo = NotificationInfoPool->Allocate();
    
    //Set the function and call Init.
    notificationInfo->iBody->iFunction = aFunction;
    TInt err = notificationInfo->Init(aFunction,aDrive);
    if(err != KErrNone)
        {
        CFsNotificationInfo::Free(notificationInfo);
        return NULL;
        }
    
    //Set request (NULL)
    notificationInfo->SetRequest(NULL);
	
	//Set UID (KNullUid)
    notificationInfo->SetUid(TUid::Null());
    return notificationInfo;
    }

EXPORT_C void CFsNotificationInfo::Free(CFsNotificationInfo*& aNotificationInfo)
    {
    __ASSERT_DEBUG(aNotificationInfo,User::Panic(_L("CFsNotificationInfo::Free - KErrArgument"), KErrArgument));
    NotificationInfoPool->Free(aNotificationInfo);    
    aNotificationInfo = NULL;
    }


TInt CFsNotificationInfo::Initialise()
    {
    //Have to trap as sf_main.cpp commonInitialize doesn't.
    TRAPD(r, NotificationInfoPool = CFsPool<CFsNotificationInfo>::New(KMaxDrives,CFsNotificationInfo::New));
    if(r != KErrNone)
        return r;
    if(!NotificationInfoPool)
        return KErrNoMemory;
    return KErrNone;
    }

CFsNotificationInfo* CFsNotificationInfo::New()
    {
    CFsNotificationInfo* info = new CFsNotificationInfo;
    __ASSERT_ALWAYS(info,Fault(ENotifyPoolCreation));
    info->iBody = new CFsNotificationInfoBody();
    __ASSERT_ALWAYS(info->iBody,Fault(ENotifyPoolCreation));
    return info;
    }

EXPORT_C TInt CFsNotificationInfo::SetSourceName(const TDesC& aSrc)
    {
    //Add the aSrc to a TParsePtrC (no copying the filename)
    TParsePtrC sourceParsePtr(aSrc);
    
    if(sourceParsePtr.DrivePresent() || !sourceParsePtr.FullName().Length())
        return KErrArgument;
    
    switch(iBody->iFunction)
        {

        case EFsFileWrite:          //EParseSrc | EFileShare
        case EFsFileSetSize:        //EParseSrc | EFileShare
        case EFsFileSetAtt:         //EParseDst | EParseSrc, - should not use these; has share.
        case EFsFileSet:            //EParseSrc | EFileShare
        case EFsFileSetModified:    //EParseSrc | EFileShare - new
        case EFsFileWriteDirty:     //EFileShare
        case EFsFileCreate:         //EParseSrc
        case EFsFileTemp:           //EParseSrc - new
        case EFsFileRename:         //EParseDst | EParseSrc,
        case EFsFileReplace:        //EParseSrc
            {
            //Should look like this:  \[path\]filename
            if(!sourceParsePtr.Name().Length())
                {
                return KErrArgument;
                }
            break;
            }
        case EFsDelete:             //EParseSrc
        case EFsSetEntry:           //EParseSrc,
        case EFsRename:             //EParseDst | EParseSrc,
        case EFsReplace:            //EParseDst | EParseSrc,
            {
            if(!sourceParsePtr.PathPresent() && !sourceParsePtr.NamePresent())
                {
                return KErrArgument;
                }
            break;
            }
        case EFsRmDir:              //EParseSrc
        case EFsMkDir:              //EParseSrc
            {
            if(!sourceParsePtr.PathPresent())
                {
                return KErrArgument;
                }
            break;
            }
      /*case EFsFormatNext:         //EParseSrc
        case EFsDismountFileSystem: //0
        case EFsMountFileSystem:    //0
        case EFsSetVolume:          //0
        case EFsSetDriveName:       //ESync
        case EFsRawDiskWrite:       //EParseSrc
        case EFsMountFileSystemScan: */
        default:
            {
            __ASSERT_DEBUG(EFalse,User::Panic(_L("CFsNotificationInfo::SetSourceName Invalid Operation"),KErrArgument));
            return KErrNotSupported;
            }
        }
    memcpy(&iBody->iSrc,&sourceParsePtr,sizeof(TParsePtrC));
    return KErrNone;
    }

EXPORT_C TInt CFsNotificationInfo::SetNewName(const TDesC& aDest)
    {
    //Add the aSrc to a TParsePtr for some validation without copying the filename
    TParsePtrC destParsePtr(aDest);
    
    if(destParsePtr.DrivePresent() || !destParsePtr.FullName().Length())
        return KErrArgument;
    
    switch(iBody->iFunction)
        {
        case EFsFileRename:         //EParseDst | EParseSrc,
        case EFsRename:             //EParseDst | EParseSrc,
        case EFsReplace:            //EParseDst | EParseSrc,
            {
            if(!destParsePtr.PathPresent() && !destParsePtr.NamePresent())
                {
                return KErrArgument;
                }
            break;
            }
        case EFsSetDriveName:
        case EFsSetVolume:
            {
            if(!destParsePtr.NamePresent())
                {
                return KErrArgument;
                }
            break;
            }            
        default:
            {
            __ASSERT_DEBUG(ETrue,User::Panic(_L("CFsNotificationInfo::SetNewName Invalid Operation"),KErrArgument));
            }
        }
    
    memcpy(&iBody->iDest,&destParsePtr,sizeof(TParsePtrC));
    return KErrNone;
    }

EXPORT_C TInt CFsNotificationInfo::SetFilesize(TInt64 aFilesize)
    {
    if(aFilesize<0)
        return KErrArgument;
    
    iBody->iData = aFilesize;
    return KErrNone;
    }
EXPORT_C TInt CFsNotificationInfo::SetAttributes(TUint aSet,TUint aCleared)
    {
    iBody->iData = MAKE_TUINT64(aSet,aCleared);
    return KErrNone;
    }

EXPORT_C TInt CFsNotificationInfo::SetUid(const TUid& aUid)
    {
    iBody->iUid = aUid;
    return KErrNone;
    }


TInt CFsNotificationInfo::SetDriveNumber(TInt aDriveNumber) 
    {
    if(aDriveNumber >= EDriveA && aDriveNumber <= EDriveZ)
        {
        iBody->iDriveNumber = aDriveNumber;
        return KErrNone;
        }
    return KErrArgument;    
    }
void CFsNotificationInfo::SetRequest(CFsRequest* aRequest)
    {
    iBody->iRequest = aRequest;
    }
TInt CFsNotificationInfo::Function() 
    { return iBody->iFunction; }
TInt CFsNotificationInfo::DriveNumber() 
    { return iBody->iDriveNumber; }
TParsePtrC& CFsNotificationInfo::Source() 
    {
    switch(iBody->iFunction)
        {
        case EFsFormatNext:         //EParseSrc
        case EFsDismountFileSystem: //0
        case EFsMountFileSystem:    //0
        case EFsSetVolume:          //0
        case EFsSetDriveName:       //ESync
        case EFsRawDiskWrite:       //EParseSrc
        case EFsMountFileSystemScan:
            {
            _LIT(KFormatDrive,"?:");
            iBody->iSrcBuf = KFormatDrive;
            iBody->iSrcBuf[0] = TText(DriveNumber() + 'A');
            TParsePtrC parse(iBody->iSrcBuf);
            memcpy(&iBody->iSrc,&parse,sizeof(TParsePtrC));
            }
        }
    return iBody->iSrc;
    }
TParsePtrC& CFsNotificationInfo::NewName() 
    { return iBody->iDest; }
CFsRequest* CFsNotificationInfo::Request() 
    { return iBody->iRequest; }
TInt64* CFsNotificationInfo::Data() 
    { return &iBody->iData; }
TUid& CFsNotificationInfo::Uid()
    { return iBody->iUid; }
TNotificationType& CFsNotificationInfo::NotificationType()
    { return iBody->iNotificationType; }
TBool CFsNotificationInfo::DestDriveIsSet()
    { return iBody->iDestDriveStored; }
TInt CFsNotificationInfo::SourceSize()
    {
    TInt size = Source().FullName().Size();
    if(NotificationType()!=TFsNotification::EMediaChange    &&
       NotificationType()!=TFsNotification::EDriveName      && 
       NotificationType()!=TFsNotification::EVolumeName)
        {
        size += sizeof(TText)*2;
        }
    return size;    
    }
TInt CFsNotificationInfo::NewNameSize()
    {
    TInt size = NewName().FullName().Size();
    if(!DestDriveIsSet())
        size += sizeof(TText)*2;
    return size;  
    }

//Get the path of the file, folder or drive name based on the TFsMessage function
void CFsNotificationInfo::PathName(CFsClientMessageRequest& aRequest, TParsePtrC& aPath)
    {
    __PRINT(_L("CFsNotificationInfo::PathName"));
    //Get the notification type
    TInt function = aRequest.Operation()->Function();
    
    //Get the filename(s)
    switch(function)
        {
        case EFsFileWrite:          //EParseSrc | EFileShare
        case EFsFileSetSize:        //EParseSrc | EFileShare
        case EFsFileSetAtt:         //EParseDst | EParseSrc, - should not use these; has share.
        case EFsFileSet:            //EParseSrc | EFileShare
        case EFsFileSetModified:    //EParseSrc | EFileShare - new
        case EFsFileWriteDirty:     //EFileShare
            {
            CFileShare* share = NULL;
            CFileCB* file = NULL;
            GetFileFromScratch(&aRequest,share,file);
            TParsePtrC ptrC(file->iFileName->Des());
            memcpy(&aPath,&ptrC,sizeof(TParsePtrC));
            break;
            }
        case EFsFileCreate:         //EParseSrc
        case EFsFileTemp:           //EParseSrc - new
        case EFsDelete:             //EParseSrc
        case EFsSetEntry:           //EParseSrc,
        case EFsFileRename:         //EParseDst | EParseSrc,
        case EFsRename:             //EParseDst | EParseSrc,
        case EFsReplace:            //EParseDst | EParseSrc,
        case EFsFileReplace:        //EParseSrc
            {
            TParsePtrC parsePtrC(aRequest.Src().FullName().Mid(2)); //Don't want drive letter
            memcpy(&aPath,&parsePtrC,sizeof(TParsePtrC));
            break;
            }
        case EFsRmDir:              //EParseSrc
        case EFsMkDir:              //EParseSrc
            {
            TParsePtrC parsePtrC(aRequest.Src().Path());
            memcpy(&aPath,&parsePtrC,sizeof(TParsePtrC));
            break;
            
            //aPath.Set(aRequest.Src().DriveAndPath(),NULL,NULL);
            //break;
            }
        case EFsFormatNext:         //EParseSrc
        case EFsDismountFileSystem: //0
        case EFsMountFileSystem:    //0
        case EFsSetVolume:          //0
        case EFsSetDriveName:       //ESync
        case EFsRawDiskWrite:       //EParseSrc
        case EFsLockDrive:
        case EFsUnlockDrive:
        case EFsReserveDriveSpace:
        case EFsMountFileSystemScan:
            {
            break;
            }
        default:
            ASSERT(0);
            break;
        }
    }

//Get the new path of the file, folder or drive name based on the TFsMessage function
void CFsNotificationInfo::NewPathName(CFsClientMessageRequest& aRequest, TParsePtrC& aNewPath)
    {
    __PRINT(_L("CFsNotificationInfo::NewPathName"));
    //Get the notification type
    TInt function = aRequest.Operation()->Function();

    //Get the filename(s)
    switch(function)
        {
        case EFsFileRename:         //EParseDst | EParseSrc,
        case EFsRename:             //EParseDst | EParseSrc,
        case EFsReplace:            //EParseDst | EParseSrc,
            {
            //We must provide the drive letter too as in the case
            //of the file being monitored being renamed to a 
            //different drive.
            //In that case we need to provide the new drive letter to the client.
            TParsePtrC ptrC(aRequest.Dest().FullName());
            memcpy(&aNewPath,&ptrC,sizeof(TParsePtrC));
            break;
            }
        case EFsSetVolume:          //EParseDst
        case EFsSetDriveName:       //ESync | EParseDst
            {
            TParsePtrC ptrC(aRequest.Dest().FullName());
            memcpy(&aNewPath,&ptrC,sizeof(TParsePtrC));
            break;
            }
        default:
            {
            ASSERT(0);
            }
        }
    }

//Get the size of the notification based on its type
TInt CFsNotificationInfo::NotificationSize(CFsNotificationInfo& aRequest)
    {
    __PRINT(_L("CFsNotificationInfo::NotificationSize"));
    
    /*
     * If there are no new names, the order of the data in the buffer is:
     * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
     * Word2   : NotificationType (Lower 2 bytes)
     * Word3   : UID
     * Word(s) : Path (TText8) , [Any sub-class members]
     * 
     * Else for notification types ERename, EVolumeName and EDriveName the order is:
     * Word1   : NotificationSize (2 bytes) , PathSize (2 bytes)
     * Word2   : NewNameSize (2 bytes) , NotificationType (2 bytes)
     * Word3   : UID
     * Word(s) : Path (TText8) , NewName (TText8)
     * 
     * EOverflow size: KNotificationHeaderSize
     */ 
    
    //Size of the filename +(with '<drive>:')
    TInt size = KNotificationHeaderSize + Align4(aRequest.SourceSize());
    
    switch(aRequest.NotificationType())
        {
        //NewName
        case TFsNotification::ERename:
        case TFsNotification::EVolumeName:
        case TFsNotification::EDriveName:
            {
            if(!aRequest.NewName().FullName().Length())
                __ASSERT_ALWAYS(false,User::Panic(_L("CFsNotificationInfo::NotificationSize"),KErrArgument));
            
            size += Align4(aRequest.NewNameSize());
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


TNotificationType CFsNotificationInfo::NotificationType(TInt& aIndex)
    {
    __PRINT(_L("CFsNotificationInfo::NotificationType(TInt)"));
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
TInt CFsNotificationInfo::TypeToIndex(TNotificationType aType)
    {
    __PRINT(_L("CFsNotificationInfo::ArrayIndex"));

    TInt index = 0; 
    switch(aType) //No break statements here on purpose
        {
        case TFsNotification::EMediaChange: index++;
        case TFsNotification::EDriveName:   index++;
        case TFsNotification::EVolumeName:  index++;
        case TFsNotification::EDelete:      index++;
        case TFsNotification::EAttribute:   index++;
        case TFsNotification::ECreate:      index++;
        case TFsNotification::ERename:      index++;
        case TFsNotification::EFileChange:  // skip;
        default: break;
        }
    __ASSERT_DEBUG(index < KNumRegisterableFilters, Fault(ENotificationFault));
    return index;
    }

TInt CFsNotificationInfo::DriveNumber(const TPtrC& aPath)
    {
    if(aPath.Length() >= 1)
        {
        TInt drive;
        TInt r = RFs::CharToDrive(aPath[0],drive);
        if(r!=KErrNone)
            return KErrNotFound;
        return drive;
        }
    return KErrNotFound;
    }

//Get the attributes set and cleared
void CFsNotificationInfo::Attributes(CFsMessageRequest& aRequest, TUint& aSet, TUint& aClear)
    {
    __PRINT(_L("CFsNotificationInfo::Attributes"));

    TInt function = aRequest.Operation()->Function();
    const RMessage2& msg = aRequest.Message();

    //Client notification
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

TInt64 CFsNotificationInfo::FileSize(CFsMessageRequest& aRequest)
    {
    CFileShare* share = NULL;
    CFileCB* file = NULL;
    GetFileFromScratch(&aRequest, share, file);
    TInt64 size = file->CachedSize64();
    return size;
    }


void CFsNotificationInfo::SetData(CFsMessageRequest* aRequest, CFsNotificationInfo* aNotificationInfo)
    {
    TInt function = aRequest->Operation()->Function();
    
    switch(function)
        {
        case EFsFileWrite:
        case EFsFileWriteDirty:
        case EFsFileSetSize:
            {
            aNotificationInfo->SetFilesize(FileSize(*aRequest));
            break;
            }
        case EFsSetEntry:
        case EFsFileSetAtt:
        case EFsFileSet:
            {
            TUint set = 0;
            TUint clear = 0;
            Attributes(*aRequest,set,clear);
            *(aNotificationInfo->Data())= MAKE_TUINT64(set,clear);
            break;
            }
        default:
            {
            return;
            }
        }
    }

TInt CFsNotificationInfo::ValidateNotification(CFsNotificationInfo& aNotificationInfo)
    {
    //Validate UID
    if(aNotificationInfo.Uid() == TUid::Null())
        {
        __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Uid not set"),KErrArgument));
        return KErrArgument;
        }
   
    switch(aNotificationInfo.Function())
        {
        case EFsFileWrite:
        case EFsFileWriteDirty:
        case EFsFileSetSize:
            {
            if(*aNotificationInfo.Data() == -1)
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - File size not set"),KErrArgument));
                return KErrArgument;
                }
            if(!aNotificationInfo.Source().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Source Name not set"),KErrArgument));
                return KErrArgument;
                }
            if(aNotificationInfo.NewName().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - New Name set in err"),KErrArgument));
                return KErrArgument;
                }
            break;
            }
        case EFsRename:
        case EFsFileRename:
        case EFsReplace:
        case EFsSetVolume:
        case EFsSetDriveName:
            {
            if(!aNotificationInfo.Source().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Source Name not set"),KErrArgument));
                return KErrArgument;
                }
            if(!aNotificationInfo.NewName().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - New Name not set"),KErrArgument));
                return KErrArgument;
                }
            if(*aNotificationInfo.Data() != -1)
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Data set in err"),KErrArgument));
                return KErrArgument;
                }
            break;
            }
        case EFsMkDir:
        case EFsFileCreate:
        case EFsFileReplace:
        case EFsDelete:
        case EFsRmDir:
            {
            if(!aNotificationInfo.Source().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Source Name not set"),KErrArgument));
                return KErrArgument;
                }
            if(aNotificationInfo.NewName().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - New Name set in err"),KErrArgument));
                return KErrArgument;
                }
            if(*aNotificationInfo.Data() != -1)
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Data set in err"),KErrArgument));
                return KErrArgument;
                }
            break;
            }
        case EFsFileSetAtt:
        case EFsFileSet:
        case EFsSetEntry:
            {
            if(!aNotificationInfo.Source().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Source Name not set"),KErrArgument));
                return KErrArgument;
                }
            if(*aNotificationInfo.Data() == -1)
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Attributes not set"),KErrArgument));
                return KErrArgument;
                }
            if(aNotificationInfo.NewName().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - New Name set in err"),KErrArgument));
                return KErrArgument;
                }               
            break;
            }
        case EFsDismountFileSystem:
        case EFsMountFileSystem:
        case EFsFormatNext:
        case EFsRawDiskWrite:
        case EFsMountFileSystemScan:
            {
            if(aNotificationInfo.NewName().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - New Name set in err"),KErrArgument));
                return KErrArgument;
                }               
            if(aNotificationInfo.Source().FullName().Length())
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Source Name set in err"),KErrArgument));
                return KErrArgument;
                }
            if(*aNotificationInfo.Data() != -1)
                {
                __ASSERT_DEBUG(false,User::Panic(_L("::ValidateNotification - Data set in err"),KErrArgument));
                return KErrArgument;
                }
            break;
            }
        default:
            {
            break;
            }
        }
    return KErrNone;
    }

TUint CFsNotificationInfo::NotifyType(TInt aFunction)
//
//  Convert aFunction that caused the notification to a 
//  value corresponding to the correct TNotifyType enum  
//
    {
    switch (aFunction)
        {
        case EFsFileCreate:
        case EFsFileReplace:
        case EFsDelete:
        case EFsReplace:
        case EFsFileRename:
            return(ENotifyFile|ENotifyEntry);   
        case EFsMkDir:
        case EFsRmDir:
            return(ENotifyDir|ENotifyEntry);    
        case EFsRename:                 
            return(ENotifyDir|ENotifyFile|ENotifyEntry);                    
        case EFsSetVolume:                  
            return(ENotifyDisk|ENotifyEntry);   
        case EFsFileSet:
        case EFsFileSetAtt:
        case EFsFileSetModified:
        case EFsFileSetSize:
        case EFsSetEntry:
            return(ENotifyAttributes);  
        case EFsFileWrite:
        case EFsFileWriteDirty:
            return(ENotifyWrite);   
        case EFsRawDiskWrite:
        case EFsLockDrive:
        case EFsUnlockDrive:
            return(ENotifyDisk);    
        case EFsFileTemp:       
        case EFsSetDriveName:
            return(ENotifyAll); 
        default:
            return(0);
        }
    }

void CNotifyInfo::Initialise(TInfoType aType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
//
//
//
	{
	iType=aType;
	iStatus=aStatus;
	iMessage=aMessage;
	iSession=aSession;
	};

CNotifyInfo::~CNotifyInfo()
//
//
//
	{
	// message should have been completed already
	__ASSERT_DEBUG(iMessage.IsNull(), Fault(ENotifyInfoDestructor));

	__ASSERT_DEBUG(iLink.iNext,Fault(ENotifyInfoDestructor));	
	iLink.Deque();
	}

void CNotifyInfo::Complete(TInt aError)
//
//
//
	{
	__PRINT2(_L("CNotifyInfo::Complete 0x%x error=%d"),this,aError);
	if (!iMessage.IsNull())						
		iMessage.Complete(aError);
	}


void CStdChangeInfo::Initialise(TNotifyType aChangeType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
//
//
//
	{
	iChangeType=aChangeType;
	CNotifyInfo::Initialise(EStdChange,aStatus,aMessage,aSession);
	}

TUint CStdChangeInfo::RequestNotifyType(CFsNotificationInfo* aRequest)
//
// return notification type for the request
//
	{
    TUint notifyType=CFsNotificationInfo::NotifyType(aRequest->Function());
    if(aRequest->Function()==EFsRename)
		{
		__ASSERT_DEBUG(notifyType==(ENotifyDir|ENotifyFile|ENotifyEntry),Fault(EStdChangeRequestType));
        if(aRequest->Source().NamePresent())
			notifyType=ENotifyFile|ENotifyEntry;
		else
			notifyType=ENotifyDir|ENotifyEntry;
		}
	return(notifyType);						
	}


TBool CStdChangeInfo::IsMatching(CFsNotificationInfo* aNotificationInfo)
//
// return ETrue if operation type of request matches that of change notification
//
    {
    if((iChangeType&ENotifyAll) || (iChangeType&CFsNotificationInfo::NotifyType(aNotificationInfo->Function())))
		return(ETrue);
	else
		return(EFalse);
	}

void CExtChangeInfo::Initialise(TNotifyType aChangeType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession,const TDesC& aName)
//
//
//
	{
	__ASSERT_DEBUG(aName.Length()<=KMaxFileName-2,Fault(EExtChangeNameLength));
	iName=aName;
	iChangeType=aChangeType;
	CNotifyInfo::Initialise(EExtChange,aStatus,aMessage,aSession);
	}


TBool CExtChangeInfo::IsMatching(CFsNotificationInfo* aRequest)
//
// return ETrue if operation notify type of request matches that of change notification
// and paths match
//
    {
    TInt function=aRequest->Function();
	//	if a rename occurred inform any requests if their path has been changed regardless of the notification type
	if(function==EFsRename)				
		{		
        TBuf<KMaxFileName> renamePath=aRequest->Source().FullName();        
		renamePath+=_L("*");
		if (iName.MatchF(renamePath)!=KErrNotFound)	
			return(ETrue);
		}


	//Special case where the dir the notifier is setup on has just been created
	if(function==EFsMkDir)	
		{		
        if(aRequest->Source().Path().MatchF(iName) == 0)
			return ETrue;
		}
	
	//Special case where  the File the notifier is setup on has just been created by temp as the name is not known until it has been created
	if(function==EFsRename||function==EFsFileOpen||function==EFsFileCreate||function==EFsFileReplace)
		{
        if(aRequest->Source().FullName().MatchF(iName) == 0)
			return ETrue;
		}
	
	//For the case of a file created using EFsFileTemp we can probably ignore it for special cases as it 
	//is created with a random name. Not specifically going to be being looked for

	if((iChangeType&ENotifyAll) || (iChangeType&RequestNotifyType(aRequest)))
		{
		switch (function)
			{	
		//	Notify interested requests if a SetDriveName(), SetVolume() or RawDiskWrite() operation
		//	occcurred.  By their nature, these operations have no distinct path.  All outstanding
		//	requests monitoring the relevant TNotifyType are potentially interested in such operations	
			case EFsFileWrite:
			case EFsFileWriteDirty:
			case EFsFileSet:
			case EFsFileSetAtt:
			case EFsFileSetModified:
			case EFsFileSetSize:
			{
			TBuf<KMaxFileName> root=iName;
			root+=_L("*");	
			
                if (aRequest->Source().FullName().MatchF(root) != KErrNotFound)
                    {
                    return(ETrue);
                    }
			}
			break;
			case EFsSetDriveName:
			case EFsSetVolume:
			case EFsRawDiskWrite:
			case EFsLockDrive:
			case EFsUnlockDrive:
			case EFsReserveDriveSpace:
				{
				return(ETrue);				
				}
			
			default:
				{	
				TBuf<KMaxFileName> root = iName;
				root+=_L("*");	
				
                if(aRequest->Source().FullName().MatchF(root)!=KErrNotFound)
					return(ETrue);	
				else if(function==EFsRename||function==EFsReplace||function==EFsFileRename)
					{
					// - rename/replace causes the file/path to disappear
				    TPtrC newName;
				    if(aRequest->DestDriveIsSet())
				        newName.Set(aRequest->NewName().FullName().Mid(2));
				    else
				        newName.Set(aRequest->NewName().FullName());
                    if(newName.MatchF(root)!=KErrNotFound)
						{
						return(ETrue);
						}

					// - rename/replace causes the file/path to arrive
                    if(aRequest->DestDriveIsSet())
                        root=aRequest->NewName().FullName().Mid(2);
                    else
                        root=aRequest->NewName().FullName();
					root+=_L("*");

					if (iName.MatchF(root)!=KErrNotFound)
						{
						return(ETrue);
						}
					}
				}			
			}
		}
	return(EFalse);
	}


void CDiskSpaceInfo::Initialise(TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession,TInt64 aThreshold)
//
//
//
	{
	__ASSERT_DEBUG(aThreshold>0,Fault(EDiskSpaceThreshold));
	iThreshold=aThreshold;
	CNotifyInfo::Initialise(EDiskSpace,aStatus,aMessage,aSession);
	}

TBool CDiskSpaceInfo::IsMatching(TInt64& aBefore,TInt64& aAfter)
//
// return ETrue if the threshold has been crossed
//
	{
	if((aBefore>=iThreshold&&aAfter<iThreshold)||(aBefore<=iThreshold&&aAfter>iThreshold))
		return(ETrue);
	return(EFalse);
	}

void CDebugChangeInfo::Initialise(TUint aDebugType,TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
//
//
//
	{
	__ASSERT_DEBUG((aDebugType&KDebugNotifyMask)&&!(aDebugType&~KDebugNotifyMask),Fault(EDebugChangeType));
	iDebugType=aDebugType;
	CNotifyInfo::Initialise(EDebugChange,aStatus,aMessage,aSession);
	}

TBool CDebugChangeInfo::IsMatching(TUint aFunction)
//
// return ETrue if debug notification type matches aFunction
//
	{
	if(iDebugType&aFunction)
		return(ETrue);
	return(EFalse);
	}


CDismountNotifyInfo::~CDismountNotifyInfo()
	{
	switch(iMode)
		{
		case EFsDismountNotifyClients:
			break;
		case EFsDismountRegisterClient:
			__ASSERT_ALWAYS(TheDrives[iDriveNumber].DismountUnlock() >= 0, Fault(ENotifyDismountCancel));
			__ASSERT_ALWAYS(iMessage.IsNull(), Fault(ENotifyDismountCancel));
			TheDrives[iDriveNumber].DismountClientRemoved();
			break;
		default:
			break;
		}
	}

void CDismountNotifyInfo::Complete(TInt aError)
	{
	__PRINT2(_L("CDismountNotifyInfo::Complete 0x%x error=%d"),this,aError);
	if (!iMessage.IsNull())						
		{
		iMessage.Complete(aError);
		// inc count of messages completed by EFsDismountNotifyClients & waiting for an EFsAllowDismount request from client
		if (iMode == EFsDismountRegisterClient)
			TheDrives[iDriveNumber].DismountClientAdded();
		}
	}


void CDismountNotifyInfo::Initialise(TNotifyDismountMode aMode, TInt aDriveNumber, TRequestStatus* aStatus,const RMessagePtr2& aMessage,CSessionFs* aSession)
	{
	iMode = aMode;
	iDriveNumber=aDriveNumber;
	CNotifyInfo::Initialise(EDismount,aStatus,aMessage,aSession);

	if (iMode == EFsDismountRegisterClient)
		TheDrives[iDriveNumber].DismountLock();
	}

TBool CDismountNotifyInfo::IsMatching(TNotifyDismountMode aMode, TInt aDriveNumber, CSessionFs* aSession)
	{
	if((iDriveNumber == aDriveNumber) && (iMode == aMode) && (aSession == NULL || aSession == Session()))
		return(ETrue);
	return(EFalse);
	}

TBaseQue::TBaseQue()
//
//
//
	{
	iHeader.SetOffset(_FOFF(CNotifyInfo,iLink));
	TInt r=iQLock.CreateLocal();
	__ASSERT_ALWAYS(r==KErrNone,Fault(EBaseQueConstruction));
	}

TBaseQue::~TBaseQue()
//
//
//
	{
	iQLock.Close();
	}

void TBaseQue::DoAddNotify(CNotifyInfo* aInfo)
//
// Add notification
// Que should be locked by calling function
//
	{
	iHeader.AddLast(*aInfo);
	}

TBool TBaseQue::DoCancelSession(CSessionFs* aSession,TInt aCompletionCode, TRequestStatus* aStatus)
//
// Cancel notification(s) setup by aSession matching aStatus if set
// Que should be locked by calling function
//
	{
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	TBool isFound=EFalse;
	while((info=q++)!=NULL)
		{
		if(info->Session()==aSession && (!aStatus || aStatus==info->Status()))
			{
			isFound=ETrue;
			info->Complete(aCompletionCode);
			delete(info);
			if(aStatus)
				break;
			}
		}
	return(isFound);
	}

void TBaseQue::DoCancelAll(TInt aCompletionCode)
//
// Cancel all notifications
// Que should be locked by calling function
//
	{
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		info->Complete(aCompletionCode);
		delete(info);
		}
	__ASSERT_DEBUG(iHeader.IsEmpty(),Fault(EBaseQueCancel));
	}

TBool TBaseQue::IsEmpty()
//
// Que should be locked by calling function
//
	{
	return iHeader.IsEmpty();
	}

TBool TChangeQue::IsEmpty()
//
//
//
	{
	iQLock.Wait();
	TBool empty = TBaseQue::IsEmpty();
	iQLock.Signal();
	return(empty);
	}
	
TInt TChangeQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(KErrNone);
	}

TBool TChangeQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
//
	{
	iQLock.Wait();
	TBool isFound=TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);
	iQLock.Signal();
	return(isFound);
	}

void TChangeQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}

void TChangeQue::CheckChange(CFsNotificationInfo& aRequest)
//
// complete any notification in que that matches aRequest
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EStdChange||info->Type()==CNotifyInfo::EExtChange,Fault(EChangeQueType));
		TBool isMatching;
		if(info->Type()==CNotifyInfo::EStdChange)
			isMatching=((CStdChangeInfo*)info)->IsMatching(&aRequest);
		else
			isMatching=((CExtChangeInfo*)info)->IsMatching(&aRequest);
		if(isMatching)
			{
			__PRINT1(_L("TChangeQue::CheckChange()-Matching info=0x%x"),info);
			info->Complete(KErrNone);
			delete(info);
			}
		}
	iQLock.Signal();
	}

TBool TDiskSpaceQue::IsEmpty()
//
//
//
	{
	iQLock.Wait();
	TBool empty = TBaseQue::IsEmpty();
	iQLock.Signal();
	return(empty);
	}

TInt TDiskSpaceQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TInt r=KErrNone;
	if(iHeader.IsEmpty())
		{
		r=GetFreeDiskSpace(iFreeDiskSpace);
		iReservedDiskSpace = TheDrives[iDriveNumber].ReservedSpace();
		}
	if(r==KErrNone)
		TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(r);
	}

TInt TDiskSpaceQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);
	iQLock.Signal();
	return(KErrNone);
	}

void TDiskSpaceQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}


void TDiskSpaceQue::CheckDiskSpace()
//
// Complete any disk space notification whose threshold has been crossed
//
	{
	iQLock.Wait();
	if(iHeader.IsEmpty())
		{
		iQLock.Signal();
		return;
		}
	TInt64 freeSpace;
	TInt r=GetFreeDiskSpace(freeSpace);
	TInt64 reservedSpace(TheDrives[iDriveNumber].ReservedSpace());
	if(r==KErrNone)
		{
		if((freeSpace==iFreeDiskSpace) && (reservedSpace==iReservedDiskSpace))
			{
			iQLock.Signal();
			return;
			}
		TDblQueIter<CNotifyInfo> q(iHeader);
		CNotifyInfo* info;
		while((info=q++)!=NULL)
			{
			__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDiskSpace,Fault(EDiskSpaceQueType1));

			TInt64 newSessionFreeSpace(freeSpace);
			TInt64 oldSessionFreeSpace(iFreeDiskSpace);
			if(!info->Session()->ReservedAccess(iDriveNumber))
				{
				newSessionFreeSpace -= reservedSpace;
				oldSessionFreeSpace -= iReservedDiskSpace;
				}

			if(((CDiskSpaceInfo*)info)->IsMatching(oldSessionFreeSpace,newSessionFreeSpace))
				{
				__PRINT1(_L("TDiskSpaceQue::CheckDiskSpace()-Matching info=0x%x"),info);
				info->Complete(KErrNone);
				delete(info);
				}
			}
		iFreeDiskSpace=freeSpace;
		iReservedDiskSpace=reservedSpace;
		}
	else
		TBaseQue::DoCancelAll(KErrNone);
	iQLock.Signal();
	}

void TDiskSpaceQue::CheckDiskSpace(TInt64& aFreeDiskSpace)
//
//
//
	{
	iQLock.Wait();
	if(iHeader.IsEmpty())
		{
		iQLock.Signal();
		return;
		}

	TInt64 reservedSpace(TheDrives[iDriveNumber].ReservedSpace());

	if((aFreeDiskSpace==iFreeDiskSpace) && (reservedSpace==iReservedDiskSpace))
		{
		iQLock.Signal();
		return;
		}
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDiskSpace,Fault(EDiskSpaceQueType2));

		TInt64 newSessionFreeSpace(aFreeDiskSpace);
		TInt64 oldSessionFreeSpace(iFreeDiskSpace);
		if(!info->Session()->ReservedAccess(iDriveNumber))
			{
			newSessionFreeSpace -= reservedSpace;
			oldSessionFreeSpace -= iReservedDiskSpace;
			}

		if(((CDiskSpaceInfo*)info)->IsMatching(oldSessionFreeSpace,newSessionFreeSpace))
			{
			__PRINT1(_L("TDiskSpaceQue::CheckDiskSpace()-Matching info=0x%x"),info);
			info->Complete(KErrNone);
			delete(info);
			}
		}
	iFreeDiskSpace=aFreeDiskSpace;
	iReservedDiskSpace=reservedSpace;
	iQLock.Signal();
	}

TInt TDiskSpaceQue::GetFreeDiskSpace(TInt64& aFreeDiskSpace)
//
// 
//
	{
	__ASSERT_DEBUG(iDriveNumber>=EDriveA&&iDriveNumber<=EDriveZ,Fault(EDiskSpaceQueDrive));
	__CHECK_DRIVETHREAD(iDriveNumber);
	TInt r=TheDrives[iDriveNumber].FreeDiskSpace(aFreeDiskSpace);
	return(r);
	}

TInt TDebugQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(KErrNone);
	}

TInt TDebugQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
//
	{
	iQLock.Wait();
	TBool isFound=TBaseQue::DoCancelSession(aSession,aCompletionCode,aStatus);
	iQLock.Signal();
	return(isFound);
	}

void TDebugQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}

void TDebugQue::CheckDebug(TUint aDebugChange)
//
// Complete any debug notification whose debug type matches aDebugChange
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDebugChange,Fault(EDebugQueType));
		if(((CDebugChangeInfo*)info)->IsMatching(aDebugChange))
			{
			__PRINT1(_L("TDebugQue::CheckDebug()-Matching info=0x%x"),info);
			info->Complete(KErrNone);
			delete(info);
			}
		}
	iQLock.Signal();
	}

TInt TDismountNotifyQue::AddNotify(CNotifyInfo* aInfo)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoAddNotify(aInfo);
	iQLock.Signal();
	return(KErrNone);
	}

void TDismountNotifyQue::CancelSession(CSessionFs* aSession,TInt aCompletionCode,TRequestStatus* aStatus)
//
//
	{
	iQLock.Wait();

	TDblQueIter<CDismountNotifyInfo> q(iHeader);
	CDismountNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		if(info->Session()==aSession && (!aStatus || aStatus==info->Status()))
			{
			TInt driveNumber = info->DriveNumber();

			info->Complete(aCompletionCode);
			TInt mode = info->Mode();

			delete info;
			info = NULL;

			// if we're cancelling a dismount request (EFsDismountNotifyClients or EFsDismountForceDismount), 
			// then we need to cancel the deferred dismount and issue a disk change notification as observers 
			// may be expecting one...
			if (mode == EFsDismountNotifyClients || mode == EFsDismountForceDismount)
				{
				TheDrives[driveNumber].SetDismountDeferred(EFalse);
				FsNotify::DiskChange(driveNumber);
				}

			if(aStatus)
				break;
			}
		}

	iQLock.Signal();
	}

void TDismountNotifyQue::CancelAll(TInt aCompletionCode)
//
//
//
	{
	iQLock.Wait();
	TBaseQue::DoCancelAll(aCompletionCode);
	iQLock.Signal();
	}

void TDismountNotifyQue::CheckDismount(TNotifyDismountMode aMode, TInt aDrive, TBool aRemove, TInt aError)
//
// Complete any dismount notifications on the specified drive.
//
	{
	iQLock.Wait();
	TDblQueIter<CNotifyInfo> q(iHeader);
	CNotifyInfo* info;
	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDismount,Fault(EBadDismountNotifyType));
		if(((CDismountNotifyInfo*)info)->IsMatching(aMode, aDrive, NULL))
			{
			__PRINT1(_L("TDismountNotifyQue::CheckDismount()-Matching info=0x%x"),info);
			info->Complete(aError);
			if(aRemove)
				delete info;
			}
		}

	__ASSERT_ALWAYS(!(aRemove && aMode == EFsDismountRegisterClient && TheDrives[aDrive].DismountLocked() > 0), Fault(EDismountLocked));

	iQLock.Signal();
	}

TBool TDismountNotifyQue::HandlePendingDismount(CSessionFs* aSession, TInt aDrive)
//
// Determine if the session has any outstanding *completed* dismount notifications on the specified drive 
// and delete them. Called from TFsAllowDismount::DoRequestL()
//
	{
	iQLock.Wait();
	TDblQueIter<CDismountNotifyInfo> q(iHeader);
	CDismountNotifyInfo* info;

	TBool entryFound = EFalse;

	while((info=q++)!=NULL)
		{
		__ASSERT_DEBUG(info->Type()==CNotifyInfo::EDismount,Fault(EBadDismountNotifyType));
		if(((CDismountNotifyInfo*)info)->IsMatching(EFsDismountRegisterClient, aDrive, aSession))
			{
			__PRINT1(_L("TDismountNotifyQue::HandlePendingDismount()-Pending info=0x%x"),info);

			if (info->Completed())
				delete info;

			entryFound = ETrue;
			}
		}
	iQLock.Signal();

	return entryFound;
	}

void FsNotify::Initialise()
//
//
//
	{
	for(TInt i=0;i<KMaxDiskQues;++i)
		{
		iDiskSpaceQues[i].SetDriveNumber(i);
		}
	}

TBool FsNotify::IsChangeQueEmpty(TInt aDrive)
//
//
//
	{
	if((iChangeQues[ChangeIndex(aDrive)].IsEmpty()) && (iChangeQues[ChangeIndex(KDriveInvalid)].IsEmpty()))
		return ETrue;

	return EFalse;
	}

TInt FsNotify::AddChange(CNotifyInfo* aInfo,TInt aDrive)
//
//
//
	{
	__ASSERT_DEBUG(aInfo->Type()==CNotifyInfo::EStdChange||aInfo->Type()==CNotifyInfo::EExtChange,Fault(EBadChangeNotifyType));
	__PRINT2(_L("FsNotify::AddChange() drive=%d,info=0x%x"),aDrive,aInfo);
	iChangeQues[ChangeIndex(aDrive)].AddNotify(aInfo);
	return(KErrNone);
	}

TBool FsNotify::IsDiskSpaceQueEmpty(TInt aDrive)
//
//
//
	{
	if(iDiskSpaceQues[aDrive].IsEmpty())
		return ETrue;

	return EFalse;
	}

TInt FsNotify::AddDiskSpace(CNotifyInfo* aInfo,TInt aDrive)
//
//
//
	{
	__ASSERT_DEBUG(aInfo->Type()==CNotifyInfo::EDiskSpace,Fault(EBadDiskNotifyType));
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ),Fault(EDiskBadIndex1));
	__PRINT2(_L("FsNotify::AddDiskSpace() drive=%d,info=0x%x"),aDrive,aInfo);
	return(iDiskSpaceQues[aDrive].AddNotify(aInfo));
	}

TInt FsNotify::AddDebug(CNotifyInfo* aDebugInfo)
//
//
//
	{
	__ASSERT_DEBUG(aDebugInfo->Type()==CNotifyInfo::EDebugChange,Fault(EBadDebugNotifyType));
	__PRINT1(_L("FsNotify::AddDebug() info=0x%x"),aDebugInfo);
	iDebugQue.AddNotify(aDebugInfo);
	return(KErrNone);
	}

TInt FsNotify::AddDismountNotify(CNotifyInfo* aDismountNotifyInfo)
//
//
//
	{
	__ASSERT_DEBUG(aDismountNotifyInfo->Type()==CNotifyInfo::EDismount,Fault(EBadDismountNotifyType));
	__PRINT1(_L("FsNotify::AddDismountNotify() info=0x%x"),aDismountNotifyInfo);
	iDismountNotifyQue.AddNotify(aDismountNotifyInfo);
	return(KErrNone);
	}


void FsNotify::HandleChange(CFsNotificationInfo& aNotificationInfo)
//
// Check whether any change notifications need to be completed due to aRequest on aDrive
//
    {
    __PRINT1(_L("FsNotify::HandleChange(TFsNotificationInfo) DriveNumber=%d"),aNotificationInfo.DriveNumber());
    if(aNotificationInfo.Request() && !aNotificationInfo.Request()->IsChangeNotify())
        return;
    iChangeQues[ChangeIndex(aNotificationInfo.DriveNumber())].CheckChange(aNotificationInfo);
    iChangeQues[ChangeIndex(KDriveInvalid)].CheckChange(aNotificationInfo);
    }
	

void FsNotify::HandleDiskSpace(CFsRequest* aRequest,TInt aDrive)
//
// Check whether any disk space notifications need to be completed due to aRequest on aDrive
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ) || aDrive==KDriveInvalid,Fault(EDiskBadIndex2));
	__PRINT2(_L("FsNotify::HandleDiskSpace() aRequest=0x%x, aDrive=%d"),aRequest,aDrive);
	
	if(!aRequest->Operation()->IsDiskSpaceNotify())
		return;
	TInt f = aRequest->Operation()->Function();
	if ((f == EFsFileWrite || f == EFsFileWriteDirty) && !((CFsClientMessageRequest*)aRequest)->IsFreeChanged())
		return;
	if (FsThreadManager::IsDriveThread(aDrive,EFalse))	
		iDiskSpaceQues[aDrive].CheckDiskSpace();
	}

void FsNotify::HandleDiskSpace(TInt aDrive, TInt64& aFreeSpace)
//
//
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ),Fault(EDiskBadIndex3));
	__PRINT1(_L("FsNotify::HandleDiskSpace() aDrive=%d"),aDrive);
	iDiskSpaceQues[aDrive].CheckDiskSpace(aFreeSpace);
	}

void FsNotify::HandleDebug(TUint aFunction)
//
// Check whether any debug notifications need to be completed due to aFunction
//
	{
	__PRINT1(_L("FsNotify::HandleDebug() aFunction=0x%x"),aFunction);
	if(!(aFunction&KDebugNotifyMask))
		return;
	iDebugQue.CheckDebug(aFunction);
	}

void FsNotify::HandleDismount(TNotifyDismountMode aMode, TInt aDrive, TBool aRemove, TInt err)
//
// Handle dismount notifications for the given drive
//
	{
	__PRINT4(_L("FsNotify::HandleDismount() aMode = %d, aDrive=%d, aRemove=%d, err=%d"),aMode,aDrive,aRemove,err);
	iDismountNotifyQue.CheckDismount(aMode, aDrive, aRemove, err);
	}

TBool FsNotify::HandlePendingDismount(CSessionFs* aSession, TInt aDrive)
//
// Checks if the session has an outstanding notification registered on the drive
//
	{
	__PRINT1(_L("FsNotify::HandlePendingDismount() aDrive=%d"),aDrive);
	return iDismountNotifyQue.HandlePendingDismount(aSession, aDrive);
	}

void FsNotify::DiskChange(TInt aDrive)
//
// Complete all notifications in queus due to a disk change
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ),Fault(EDiskChangeDrive));
	__PRINT1(_L("FsNotify::DiskChange() aDrive=%d"),aDrive);
	iChangeQues[ChangeIndex(aDrive)].CancelAll(KErrNone);
	iChangeQues[ChangeIndex(KDriveInvalid)].CancelAll(KErrNone);
	iDiskSpaceQues[aDrive].CancelAll(KErrNone);
	iDebugQue.CancelAll(KErrNone);

	// if there are any files containing dirty data, start issuing write-dirty data requests to trigger
	// a critical notifier (CFileCache::HandleWriteDirtyError())
	// otherwise purge all file caches
	TDrive& drive=TheDrives[aDrive];
	drive.FlushCachedFileInfo(ETrue);	
	}

	
void FsNotify::CancelChangeSession(CSessionFs* aSession,TRequestStatus* aStatus)
//
//	Cancel change notifcation(s) setup by aSession and matching aStatus if not NULL
//
	{
	__PRINT2(_L("FsNotify::CancelChangeSession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	for(TInt i=0;i<KMaxNotifyQues;++i)
		{
		TBool isFound=iChangeQues[i].CancelSession(aSession,KErrCancel,aStatus);
		if(aStatus && isFound)
			break;
		}
	}

void FsNotify::CancelDiskSpaceSession(CSessionFs* aSession,TRequestStatus* aStatus)
//
// Cancel disk space notification(s) setup by aSession and matching aStatus if not NULL
//

	{
	__PRINT2(_L("FsNotify::CancelDiskSpaceSession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	for(TInt i=0;i<KMaxDiskQues;++i)
		{
		TBool isFound=iDiskSpaceQues[i].CancelSession(aSession,KErrCancel,aStatus);
		if(aStatus && isFound)
			break;
		}
	}

void FsNotify::CancelDebugSession(CSessionFs* aSession, TRequestStatus* aStatus)
//
// Cancel debug notification(s) setup by aSession and matching aStatus if not NULL
//
	{
	__PRINT2(_L("FsNotify::CancelDebugSession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	iDebugQue.CancelSession(aSession,KErrCancel,aStatus);
	}

void FsNotify::CancelDismountNotifySession(CSessionFs* aSession, TRequestStatus* aStatus)
//
// Cancel all media removal notification(s) setup by aSession (if aStatus == NULL)
// else cancels all outstanding notifications(s) for the session
//
	{
	__PRINT2(_L("FsNotify::CancelDismountNotifySession() aSession=0x%x aStatus=0x%x"),aSession,aStatus);
	iDismountNotifyQue.CancelSession(aSession,KErrCancel,aStatus);
	}

void FsNotify::CancelSession(CSessionFs* aSession)
//
//
//
	{
	__PRINT(_L("FsNotify::CancelSession"));
	FsNotify::CancelChangeSession(aSession);
	FsNotify::CancelDiskSpaceSession(aSession);
	FsNotify::CancelDebugSession(aSession);
	FsNotify::CancelDismountNotifySession(aSession);
	}


TInt FsNotify::ChangeIndex(TInt aDrive)
//
//
//
	{
	__ASSERT_DEBUG((aDrive>=EDriveA && aDrive<=EDriveZ) || aDrive==KDriveInvalid,Fault(EChangeBadIndex));
	if(aDrive==KDriveInvalid)
		return(0);
	else
		return(aDrive+1);
	}

