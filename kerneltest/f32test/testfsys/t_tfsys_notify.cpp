// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testfsys\t_tfsys.cpp
// 
//

#include "t_tfsys_notify.h"


const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;

void CTestNotificationMountCB::FileOpenL(const TDesC& aName,TUint /*aMode*/,TFileOpen /*anOpen*/,CFileCB* /*aFile*/)
    {
    RDebug::Print(_L("CTestMountCB::FileOpenL - aName = %S"),&aName);
    
    if(aName.Match(_L("\\Wellformed_functionReplace.txt"))!= KErrNotFound) //PhantomFile_functionWrite.txt
        {
        CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(*this,EFsFileReplace);
        TBuf<KMaxFileName> phantomName;
        phantomName.Append(_L("\\"));
        phantomName.Append(_L("PhantomFile_functionReplace.txt"));
        TInt r = notificationInfo->SetSourceName(phantomName);
        User::LeaveIfError(r);
        r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
        User::LeaveIfError(r);
        r = IssueNotification(notificationInfo);
        User::LeaveIfError(r);
        CFsNotificationInfo::Free(notificationInfo);
        return;
        }
    else if(aName.Match(_L("\\Extended_Replaced.txt"))!= KErrNotFound) //\\Extended_Replaced.txt
        {
        CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(*this,EFsFileReplace);
        TBuf<KMaxFileName> phantomName;
        phantomName.Append(_L("\\"));
        phantomName.Append(_L("PhantomExtended_Replaced.txt"));
        TInt r = notificationInfo->SetSourceName(phantomName);
        User::LeaveIfError(r);
        r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
        User::LeaveIfError(r);
        r = IssueNotification(notificationInfo);
        User::LeaveIfError(r);
        CFsNotificationInfo::Free(notificationInfo);
        return;       
        }
    else if(aName.Match(_L("\\Extended_RenameMe.txt"))!= KErrNotFound) //\\PhantomExtended_Renamed.txt
        {
        CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(*this,EFsRename);
        TBuf<KMaxFileName> phantomName;
        phantomName.Append(_L("\\"));
        phantomName.Append(_L("PhantomExtended_RenameMe.txt"));
        TInt r = notificationInfo->SetSourceName(phantomName);
        User::LeaveIfError(r);
        TBuf<KMaxFileName> phantomRename;
        phantomRename.Append(_L("\\"));
        phantomRename.Append(_L("PhantomExtended_Renamed.txt"));
        r = notificationInfo->SetNewName(phantomRename);
        User::LeaveIfError(r);
        r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
        User::LeaveIfError(r);
        r = IssueNotification(notificationInfo);
        User::LeaveIfError(r);
        CFsNotificationInfo::Free(notificationInfo);
        return;       
        }
    
 
    }

void CTestNotificationFileCB::RenameL(const TDesC& /*aNewName*/)
    {
    RDebug::Print(_L("CTestNotificationFileCB::RenameL - aName = %S"),iFileName);
    
    if(iFileName->Match(_L("\\Wellformed_functionReplace.txt"))!= KErrNotFound) //PhantomFile_functionWrite.txt
         {
         CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(Mount(),EFsFileRename);
         TBuf<KMaxFileName> phantomName;
         phantomName.Append(_L("\\"));
         phantomName.Append(_L("PhantomFile_functionReplace.txt"));
         TInt r = notificationInfo->SetSourceName(phantomName);
         User::LeaveIfError(r);
         TBuf<KMaxFileName> phantomNewName;
         phantomNewName.Append(_L("\\"));
         phantomNewName.Append(_L("PhantomFile_functionRename.txt"));
         r = notificationInfo->SetNewName(phantomNewName);
         User::LeaveIfError(r);
         r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
         User::LeaveIfError(r);
         r = Mount().IssueNotification(notificationInfo);
         User::LeaveIfError(r);
         CFsNotificationInfo::Free(notificationInfo);
         return;
         }
    
    //MALFORMED
    if(iFileName->Match(_L("\\Malformed_functionWrite.txt"))!=KErrNotFound)
        {
        CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(Mount(),EFsFileRename);
        TBuf<KMaxFileName> phantomName;
        phantomName.Append(_L("\\"));
        phantomName.Append(_L("Malformed_functionWrite.txt"));
        TInt r = notificationInfo->SetSourceName(phantomName);
        User::LeaveIfError(r);
        // Don't set NewName - Should cause error.
        //      SetNewName(phantomNewName);
        r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
        User::LeaveIfError(r);
        r = Mount().IssueNotification(notificationInfo);
        User::LeaveIfError(r);
        CFsNotificationInfo::Free(notificationInfo);
        return;
        }
    }

void CTestNotificationFileCB::WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/)
    {
    if(iFileName->Match(_L("\\Wellformed_functionWrite.txt"))!= KErrNotFound) //PhantomFile_functionWrite.txt
         {
         RDebug::Printf("CTestNotificationFileCB::WriteL - wellformed_functionWrite.txt");
         CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(Mount(),EFsFileWrite);
         TBuf<KMaxFileName> phantomName;
         phantomName.Append(_L("\\"));
         phantomName.Append(_L("PhantomFile_functionWrite.txt"));
         TInt r = notificationInfo->SetSourceName(phantomName);
         User::LeaveIfError(r);
         r = notificationInfo->SetFilesize((TInt64)4);
         User::LeaveIfError(r);
         r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
         User::LeaveIfError(r);
         r = Mount().IssueNotification(notificationInfo);
         User::LeaveIfError(r);
         CFsNotificationInfo::Free(notificationInfo);
         return;
         }
    
    //MALFORMED
    if(iFileName->Match(_L("\\Malformed_functionWrite.txt"))!=KErrNotFound)
        {
        RDebug::Printf("CTestNotificationFileCB::WriteL - malformed_functionWrite.txt");
        CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(Mount(),EFsFileWrite);
        TBuf<KMaxFileName> phantomName;
        phantomName.Append(_L("\\"));
        phantomName.Append(_L("PhantomFileMalformed_functionWrite.txt"));
        TInt r = notificationInfo->SetSourceName(phantomName);
        User::LeaveIfError(r);
        //We won't set filesize - which should result in an error.
        //  SetFilesize((TInt64)4);
        //
        r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
        User::LeaveIfError(r);
        r = Mount().IssueNotification(notificationInfo);
        User::LeaveIfError(r);
        CFsNotificationInfo::Free(notificationInfo);
        return;
        }
            
    }

void CTestNotificationFileCB::SetEntryL(const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/)
    {
    if(iFileName->Match(_L("\\Wellformed_functionAttributes.txt"))!= KErrNotFound) //PhantomFile_functionWrite.txt
         {
         CFsNotificationInfo* notificationInfo = CFsNotificationInfo::Allocate(Mount(),EFsFileSetAtt);
         TBuf<KMaxFileName> phantomName;
         phantomName.Append(_L("\\"));
         phantomName.Append(_L("PhantomFile_functionAttributes.txt"));
         TInt r = notificationInfo->SetSourceName(phantomName);
         User::LeaveIfError(r);
         r = notificationInfo->SetAttributes(KEntryAttSystem,KEntryAttHidden);
         User::LeaveIfError(r);
         r = notificationInfo->SetUid(TUid::Uid(KErrUnknown));
         User::LeaveIfError(r);
         r = Mount().IssueNotification(notificationInfo);
         User::LeaveIfError(r);
         CFsNotificationInfo::Free(notificationInfo);
         return;
         }
    }

CTestFileSystem::CTestFileSystem()
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CNotifyTestFileSystem"));
	}

CTestFileSystem::~CTestFileSystem()
//
// Destructor
//
	{}

TInt CTestFileSystem::Install()
//
// Install the file system
//
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KF32BuildVersionNumber);
	TPtrC name=_L("CNotifyTestFileSystem");
	return(SetName(&name));
	}

CMountCB* CTestFileSystem::NewMountL() const
//
// Create a new mount control block
//
	{
	return (new(ELeave) CTestNotificationMountCB);
	}

CFileCB* CTestFileSystem::NewFileL() const
//
// Create a new file
//
	{
	return (new(ELeave) CTestNotificationFileCB);
	}

CDirCB* CTestFileSystem::NewDirL() const
//
// create a new directory lister
//
	{
	return (new(ELeave) CTestDirCB);
	}

CFormatCB* CTestFileSystem::NewFormatL() const
//
// Create a new media formatter
//
	{
	return (new(ELeave) CTestFormatCB);
	}

TInt CTestFileSystem::DefaultPath(TDes& aPath) const
//
// Return the intial default path
//
	{
	aPath=_L("C:\\");
	return (KErrNone);
	}


/**
Reports whether the specified interface is supported - if it is,
the supplied interface object is modified to it

@param aInterfaceId     The interface of interest
@param aInterface       The interface object
@return                 KErrNone if the interface is supported, otherwise KErrNotFound 

@see CFileSystem::GetInterface()
*/
TInt CTestFileSystem::GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput)
    {
    switch(aInterfaceId)
        {
        case CFileSystem::EProxyDriveSupport: // The FAT Filesystem supports proxy drives
			return KErrNone;

        default:
            return(CFileSystem::GetInterface(aInterfaceId, aInterface, aInput));
        }
    }

CFileSystem* CTestFileSystem::NewL()
//
//
//
	{
	CFileSystem* testFSys = new(ELeave) CTestFileSystem;
	return testFSys;
	}


CTestNotificationMountCB::CTestNotificationMountCB(){};
CTestNotificationMountCB::~CTestNotificationMountCB(){};
CTestDirCB::CTestDirCB(){};
CTestDirCB::~CTestDirCB(){};
CTestNotificationFileCB::CTestNotificationFileCB(){};
CTestNotificationFileCB::~CTestNotificationFileCB(){};
CTestFormatCB::CTestFormatCB(){};
CTestFormatCB::~CTestFormatCB(){};


extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	return(CTestFileSystem::NewL());
	}
}

