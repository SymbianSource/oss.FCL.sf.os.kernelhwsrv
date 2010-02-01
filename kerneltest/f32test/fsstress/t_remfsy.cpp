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
// f32test\fsstress\t_remfsy.cpp
// 
//

#if defined(_UNICODE)
#if !defined(UNICODE)
#define UNICODE
#endif
#endif

/*
#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
 #include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#include <stdlib.h>
*/
#include <f32file.h>
#include <f32fsys.h>
#include <f32ver.h>
#include <e32twin.h>
#include <e32uid.h>

#include "t_remfsy.h"

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;
//
//const TInt KKiloBytes=0x400;
//const TBuf<4> KUidName=_L(":UID");
//const TInt KUidNameLength=4;
//
//LOCAL_D TBuf8<0x1000> buf;
//LOCAL_D RConsole theConsole;


//////////////////////////////////////////////////////////////////////////
//								CRemote									//
//////////////////////////////////////////////////////////////////////////	


CRemote::CRemote()
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CRemote"));
	}



TInt CRemote::Install()
//
// Install the file system.
//
	{

//	SetErrorMode(SEM_FAILCRITICALERRORS);
	RDebug::Print(_L("BASH in CRemote::Install"));
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KF32BuildVersionNumber);
    TPtrC name=_L("T_REMFSY");
	return(SetName(&name));
	}


//???JCS:  Removed all aSession parameters to get the code to compile



CMountCB* CRemote::NewMountL(/*CSessionFs* aSession*/) const
//
// Create a new mount control block.
//
	{

	return(new(ELeave) CRemoteMountCB);
	}

CFileCB* CRemote::NewFileL(/*CSessionFs* aSession*/) const
//
// Create a new file.
//
	{

	return(new(ELeave) CRemoteFileCB);
	}

CDirCB* CRemote::NewDirL(/*CSessionFs* aSession*/) const
//
// Create a new directory lister.
//
	{

	return(new(ELeave) CRemoteDirCB(/*aSession*/));
	}

CFormatCB* CRemote::NewFormatL(/*CSessionFs* aSession*/) const
//
// Create a new media formatter.
//
	{

	return(new(ELeave) CRemoteFormatCB(/*aSession*/));
	}

TInt CRemote::DefaultPath(TDes& aPath) const
//
// Return the initial default path.
//
	{

	aPath=_L("Q:\\");
	return(KErrNone);
	}


CFileSystem* CRemote::NewL()
//
//	JCS
//
	{
	CFileSystem* remoteFsy=new(ELeave) CRemote();
	return remoteFsy;
	}


void CRemote::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
//
// Return the drive info, iDriveAtt already set
//
	{

	TFileName d;
	d.Format(_L("%c:\\"),aDriveNumber+'A');
	if (MapDriveInfo(anInfo,aDriveNumber))
		{
		if (MapDriveAttributes(anInfo.iDriveAtt,aDriveNumber))
			{
			return;
			}
		}

//	UINT r=GetDriveType((_STRC)d.PtrZ());
	TMediaType t;
	anInfo.iMediaAtt=0;
    /*
	switch (r)
		{
	case DRIVE_REMOVABLE:
		anInfo.iMediaAtt|=KMediaAttDualDensity;
		t=EMediaFloppy;
		break;
	case DRIVE_NO_ROOT_DIR: t=EMediaNotPresent; break;
	case DRIVE_FIXED: t=EMediaHardDisk; break;
	case DRIVE_REMOTE: t=EMediaRemote; break;
	case DRIVE_CDROM: t=EMediaCdRom; break;
	case DRIVE_RAMDISK: t=EMediaRam; break;
	case DRIVE_UNKNOWN:
	default:
		t=EMediaUnknown;
		}
	*/
	t=EMediaRemote;
	anInfo.iType=t;
	}


extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	return(CRemote::NewL());
//	return(new CRemote);
	}
}


