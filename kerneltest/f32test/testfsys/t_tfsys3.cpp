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
// f32test\testfsys\t_tfsys3.cpp
// 
//

#include "t_tfsys3.h"


const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;

CTestFileSystem::CTestFileSystem()
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CTestFileSystem3"));
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
	TPtrC name=_L("Test3");
	return(SetName(&name));
	}

CMountCB* CTestFileSystem::NewMountL() const
//
// Create a new mount control block
//
	{
	return (new(ELeave) CTestMountCB);
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


CTestMountCB::CTestMountCB(){};
CTestMountCB::~CTestMountCB(){};
CTestDirCB::CTestDirCB(){};
CTestDirCB::~CTestDirCB(){};
CTestFileCB::CTestFileCB(){};
CTestFileCB::~CTestFileCB(){};
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

TInt CTestMountCB::ClusterSize() const
	{
	return KTestClusterSize;
	}

TInt CTestMountCB::SubType(TDes& aName) const
	{
	if(aName.MaxLength() < 12)
		return KErrArgument;

	aName = _L("Test3SubType");
	return KErrNone;
	}

TInt CTestMountCB::GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput)
	{
	switch(aInterfaceId)
		{
		case (CMountCB::EGetFileSystemSubType):
			{
			aInterface = (MFileSystemSubType*) (this);
			return KErrNone;
			}
		case (CMountCB::EGetClusterSize):
			{
			aInterface = (MFileSystemClusterSize*) (this);
			return KErrNone;
			}
		default:
			{
			return(CMountCB::GetInterface(aInterfaceId, aInterface, aInput));
			}
		}
	}
