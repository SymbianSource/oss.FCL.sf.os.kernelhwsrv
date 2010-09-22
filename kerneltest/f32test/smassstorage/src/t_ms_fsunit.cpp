// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Unit tests for the CMassStorageFileSystem class
// 
//

/**
 @file
 @internalTechnology
*/

#include <f32file.h>
#include <f32fsys.h>
#include <e32test.h>
#include "t_ms_main.h"
#include "t_ms_common.h"
#include "cmassstoragefilesystem.h"
#include "cmassstoragemountcb.h"

// a: Acutally error code;  e: Expected error code
#define LOG_AND_TEST(a, e) {if (a!=e) {test.Printf(_L("%d\n\r"), a); test(EFalse);}}

_LIT(KMsFsyName, "MassStorageFileSystem");
LOCAL_D TChar driveLetter;

LOCAL_D TBusLocalDrive* localDrive=NULL;

LOCAL_C void ParseCommandArguments()
//
// Parses the command line arguments
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	
	TPtrC token;
	token.Set(lex.NextToken());
	if (token.Length() != 0)
		{
		driveLetter = token[0];
		driveLetter.UpperCase();
		test.Printf(_L("CmdLine Param=%S"),&token);
		}
	else
		{
		test.Printf(_L("Not enough command line arguments"));
		test(EFalse);
		}
	}
	
LOCAL_C void doMsFsUnitTest()
	{
	__UHEAP_MARK;
	ParseCommandArguments();

	RFs fs;
	
	TInt err;
	TInt driveNumber;
	err = fs.CharToDrive(driveLetter, driveNumber);
	test(KErrNone == err);
	fs.Close();
	
	test.Printf(_L("Checking if drive %d is removable\n\r"), driveNumber);
	TBool removable = isDriveRemovable(driveNumber);
	if (!removable)
		{
		test.Printf(_L("This test is not supported on the specified drive\n\t"));
		return;
		}
	
	CMassStorageFileSystem* msfs = CMassStorageFileSystem::NewL();

	test.Printf(_L("Calling Install\n\r"));
	err = msfs->Install();
	test(err == KErrNone);
	
	test.Printf(_L("Comparing name\n\r"));
	TName name = msfs->Name();
	test.Printf(_L("Name is %S\n\r"), &name);
	test(name == KMsFsyName);

	test.Printf(_L("Checking version\n\r"));
	TVersion ver(1, 0, 0);
	test(msfs->QueryVersionSupported(ver));
	
	test.Printf(_L("Checking IsExtensionSupported\n\r"));
	test(msfs->IsExtensionSupported());
	
	test.Printf(_L("Checking NewMountL\n\r"));
	//In this case new mount should leave because the controller thread is not started
	TRAP(err, msfs->NewMountL());
    LOG_AND_TEST(err, KErrNotReady);
			
	test.Printf(_L("Calling unsupported functions\n\r"));
	TRAP(err, msfs->NewFileL());
	test(err == KErrNotReady);

	TRAP(err, msfs->NewDirL());
	test(err == KErrNotReady);

	TRAP(err, msfs->NewFormatL());
	test(err == KErrNotReady);

	TBuf<1> buf;
	err = msfs->DefaultPath(buf);
	test(err == KErrNotSupported);

	TDriveList list;
	err = msfs->DriveList(list);
	test(err == KErrNotSupported);
	
	test.Printf(_L("Deleting file system object\n\r"));
	msfs->Close();

	delete msfs;
	delete localDrive;	
	
	__UHEAP_MARKEND;

    test.Printf(_L("MSFS unit test ===>PASS\n"));
	}
	

GLDEF_C void t_ms_fsunit()
//
// Do all tests
//
	{
    doMsFsUnitTest();
    }

//=========================================================

EXPORT_C TInt CFsObject::SetName(const TDesC *aName)

	{
	User::Free(iName);
	iName=NULL;
	if (aName!=NULL)
		{
		iName=aName->Alloc();
		if (iName==NULL)
			return(KErrNoMemory);
		}
	return(KErrNone);
	}

EXPORT_C TName CFsObject::Name() const

	{
	if (iName)
		return(*iName);
	return(NULL);
	}
EXPORT_C CFsObject::CFsObject()

	{
	iAccessCount=1;
	}

EXPORT_C  CFsObject::~CFsObject()
	{
	if(iName)
		User::Free(iName);
	iName=NULL;
	}	

EXPORT_C TBool CFsObject::IsCorrectThread()
	{
	return(ETrue);
	}

EXPORT_C void CFsObject::Close()	
	{
	if(iName)
		User::Free(iName);
	iName = NULL;
	}
	
EXPORT_C TInt CFsObject::Open()
	{
	return KErrNone;
	}
//------------------------------------	
EXPORT_C CFileSystem::CFileSystem(void)
	{
	}
EXPORT_C CFileSystem::~CFileSystem(void)
	{
	}
	

TInt CFileSystem::DefaultPath(TDes& /*aPath*/) const 
	{
	return KErrNone;
	}

EXPORT_C void CFileSystem::DriveInfo(TDriveInfo& /* aInfo */, TInt /* aDriveNumber */) const
	{
	}

EXPORT_C TBool CFileSystem::IsExtensionSupported() const
	{
	return(EFalse);
	}

EXPORT_C TBool CFileSystem::QueryVersionSupported(const TVersion& aVer) const
	{

	return(User::QueryVersionSupported(iVersion,aVer));
	}
	
EXPORT_C TInt CFileSystem::Remove()
	{

	return(KErrNone);
	}	

//--------------------------------------------------
EXPORT_C TBool IsValidLocalDriveMapping(TInt /*aDrive*/)
//
// Is the drive number to local drive mapping valid?
//
	{

	return(ETrue);
	}



EXPORT_C TBusLocalDrive& GetLocalDrive(TInt aLocalDrive)
	{
	
	TBusLocalDrive* dd=new(ELeave) TBusLocalDrive;
	TBool mediaCh;
	
	dd->Connect(aLocalDrive, mediaCh);
	localDrive = dd;
	return *dd;

	
	}
	
EXPORT_C TBool DriveNumberToLocalDriveNumber(TInt aDrive)
	{
	return aDrive;
	}

//---------------------------------------------
EXPORT_C CLocDrvMountCB::CLocDrvMountCB() {}

EXPORT_C CLocDrvMountCB::~CLocDrvMountCB()

	{
	if(iProxyDrive)
		delete(iProxyDrive);
	}
	
//-----------------------------------
EXPORT_C CMountCB::CMountCB()
	: iMountQ(_FOFF(CFileCB,iMountLink))
	{

	}

/**
Destructor.

Frees resources before destroying the object.
*/
EXPORT_C CMountCB::~CMountCB()
	{
	delete iVolumeName;
	}	

//-------------------------------------
CFsDispatchObject::CFsDispatchObject()

:iDriveNumber(-1) 
{}

CFsDispatchObject::~CFsDispatchObject()
	{
	}
EXPORT_C int CFsDispatchObject::IsCorrectThread(void)
	{
	return ETrue;
	}

EXPORT_C void CFsDispatchObject::Close()
	{
	}	

EXPORT_C void CMountCB::IsFileInRom(const TDesC& /*aFileName*/,TUint8*& aFileStart)
	{
	aFileStart=NULL;
	}

EXPORT_C int CLocDrvMountCB::CreateLocalDrive(class TBusLocalDrive &) 

	{
	return KErrNone;
	}
EXPORT_C void CLocDrvMountCB::DismountedLocalDrive(void) 
	{
	}
	
EXPORT_C void WriteToDisk(const TDesC& /*aFileName*/, const TDesC8& /*aBuf*/)
	{
	}

// Implement the GetInterface methods here as these are usually 
// exported by EFILE, but these unit tests don't link to it.

EXPORT_C TInt CMountCB::GetInterface(TInt /*aInterfaceId*/, TAny*& /*aInterface*/, TAny* /*aInput*/)
	{
	return KErrNotSupported;
	}

EXPORT_C TInt CFileSystem::GetInterface(TInt /*aInterfaceId*/, TAny*& /*aInterface*/, TAny* /*aInput*/)
	{
	return KErrNotSupported;
	}

