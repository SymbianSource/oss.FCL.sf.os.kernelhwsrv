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
// f32test\fsstress\t_remmnt.cpp
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



LOCAL_C TInt GetMediaSize(TInt /*aDriveNumber*/,TInt64& /*aSize*/,TInt64& /*aFree*/)
//
// Return the size and free space on a drive.
//
	{
	return(KErrNone);
	}

LOCAL_C TInt GetVolume(TInt /*aDriveNumber*/,TDes& aName,TUint& aUniqueID)
//
// Return the volume name and uniqueID.
//
	{
	aUniqueID=1234;
	aName=(_L("REMOTDRV"));
	return(KErrNone);	
	}


//////////////////////////////////////////////////////////////////////////
//								CRemoteMountCB							//
//////////////////////////////////////////////////////////////////////////	


CRemoteMountCB::CRemoteMountCB()
//
// Constructor
//
	{
	__DECLARE_NAME(_S("CRemoteMountCB"));
	}

CRemoteMountCB::~CRemoteMountCB()
//
// Destructor
//
	{}

void CRemoteMountCB::MountL(TBool /*aForceMount*/)
//
// Mount a media. Only allowed to leave with KErrNoMemory,KErrNotReady,KErrCorrupt,KErrUnknown.
//
	{
//	TInt64 s,f;
	TFileName driveName;
	TInt d=Drive().DriveNumber();
//	TInt driveNum=d;
	if (MapDrive(driveName,d))
		RFs::CharToDrive(driveName[0],d);
	//User::LeaveIfError(GetMediaSize(d,s,f));
	//if (driveNum==EDriveZ)
//		iSize=4*1048576;
//	else
		//iSize=s;
	iSize=4*1024*16;
	User::LeaveIfError(GetVolume(d,driveName,iUniqueID));
	HBufC* pN=driveName.AllocL();
	SetVolumeName(pN);
	}

TInt CRemoteMountCB::ReMount()
//
// Try and remount this media.
//
	{

	TFileName n;
	TInt d=Drive().DriveNumber();
	if (MapDrive(n,d))
		RFs::CharToDrive(n[0],d);
	TUint uniqueID;
	TInt r=GetVolume(d,n,uniqueID);
	if (r!=KErrNone)
		return(r);
	if (n==VolumeName() && uniqueID==iUniqueID)
		return(KErrNone);
	return(KErrGeneral);
	}

void CRemoteMountCB::Dismounted()
//
//	Dummy implementation of a pure virtual function
//
	{}

void CRemoteMountCB::VolumeL(TVolumeInfo& aVolume) const
//
// Return the volume info.
//
	{
	TInt64 s,f(0);
	TFileName n;
	TInt d=Drive().DriveNumber();
	TInt driveNum=d;
	if (MapDrive(n,d))
		RFs::CharToDrive(n[0],d);
	User::LeaveIfError(GetMediaSize(d,s,f));
	if (driveNum==EDriveZ)
		aVolume.iFree=0;
	else
		aVolume.iFree=f;
	}


void CRemoteMountCB::SetVolumeL(TDes& /*aName*/)
//
//	Set the volume label
//	Dummy implementation of a pure virtual function
//
	{}


void CRemoteMountCB::IsFileInRom(const TDesC& /*aName*/,TUint8*& /*aFileStart*/)
//
// Return the address of the file if it is in rom
//
	{}




void CRemoteMountCB::MkDirL(const TDesC& /*aName*/)
//
//	Make a directory
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}


void CRemoteMountCB::RmDirL(const TDesC& /*aName*/)
//
//	Remove a directory
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//	Wait 0.2 seconds
	}

void CRemoteMountCB::DeleteL(const TDesC& /*aName*/)
//
//	Delete a file
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}

void CRemoteMountCB::RenameL(const TDesC& /*anOldName*/,const TDesC& /*aNewName*/)
//
//	Rename a file or directory
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}

void CRemoteMountCB::ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*aNewName*/)
//
//	Delete aNewName if it exists and rename anOldName
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}

void CRemoteMountCB::ReadUidL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const
//
//	Read the entry uid if present
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}


void CRemoteMountCB::EntryL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const
//
//	Get entry details
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}

void CRemoteMountCB::SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/)
//
//	Set entry details
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}


void CRemoteMountCB::FileOpenL(const TDesC& /*aName*/,TUint /*aMode*/,TFileOpen /*anOpen*/,CFileCB* /*aFile*/)
//
//	Open a File
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}

void CRemoteMountCB::DirOpenL(const TDesC& /*aName*/,CDirCB* /*aDir*/)
//
//	Open a directory on the current mount
//	Dummy implementation of a pure virtual function
//
	{
	User::After(200000);	//  Wait 0.2 seconds
	}


void CRemoteMountCB::RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const
//
//	Read directly from disk
//
	{
	User::Leave(KErrNotSupported);
	}

void CRemoteMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
//
//	Write directly to disk
//
	{
	User::Leave(KErrNotSupported);
	}

void CRemoteMountCB::GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/)
//
//	Get the short name associated with aLongName
//	Dummy implementation of a pure virtual function
//
	{}


void CRemoteMountCB::GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/)
//
//	Get the short name associated with aLongName
//	Dummy implementation of a pure virtual function
//
	{}


void CRemoteMountCB::ReadSectionL(const TDesC& /*aName*/,TInt /*aPos*/,TAny* /*aTrg*/,TInt /*aLength*/,const RMessagePtr2& /*aMessage*/)
//
//	Get the short name associated with aLongName
//	Dummy implementation of a pure virtual function
//
	{}



TBool CRemoteMountCB::IsRomDrive() const
//
// Returns ETrue if the drive == EDriveZ
//
	{
	return(Drive().DriveNumber()==EDriveZ);
	}



