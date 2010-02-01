// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testfsys\t_tfsys2.cpp
// 
//

#include "t_tfsys2.h"


const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;

//TInt gMountAttempts = 0;

CTestFileSystem::CTestFileSystem()
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CTestFileSystem2"));
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
	TPtrC name=_L("Test2");
	return(SetName(&name));
	}

CMountCB* CTestFileSystem::NewMountL() const
//
// Create a new mount control block
//
	{
	CTestMountCB* mount = new(ELeave) CTestMountCB;
	mount->SetFsy(this);
	return  mount;
	}

CFileCB* CTestFileSystem::NewFileL() const
//
// Create a new file
//
	{
	return (new(ELeave) CTestFileCB);
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

void CTestFileSystem::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
//
// Return drive info
//
	{
    CFileSystem::DriveInfo(anInfo, aDriveNumber);

	// hijack the iBattery member to report back the number of times MountL() has been called
	anInfo.iBattery = (TBatteryState) iMountAttempts;
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


CTestMountCB::CTestMountCB() : iFileSystem(NULL)
	{
	};

CTestMountCB::~CTestMountCB(){};
CTestDirCB::CTestDirCB(){};
CTestDirCB::~CTestDirCB(){};
CTestFileCB::CTestFileCB(){};
CTestFileCB::~CTestFileCB(){};
CTestFormatCB::CTestFormatCB(){};
CTestFormatCB::~CTestFormatCB(){};
void CTestMountCB::MountL(TBool /*aForceMount*/)
	{
	if (iFileSystem)
		iFileSystem->iMountAttempts++;

	User::Leave(KErrCorrupt);
	}

void CTestMountCB::EntryL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const
	{
	User::Leave(KErrNotFound);
	}

TInt CTestMountCB::ForceRemountDrive(const TDesC8* /*aMountInfo*/,TInt /*aMountInfoMessageHandle*/,TUint /*aFlags*/)
	{
	Drive().SetChanged(ETrue);
	return(KErrNone);
	}

void CTestMountCB::SetFsy(const CTestFileSystem* aFileSystem)
	{
	iFileSystem = const_cast<CTestFileSystem*> (aFileSystem);
	}

extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	return(CTestFileSystem::NewL());
	}
}

