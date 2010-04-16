// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class declaration for CMassStorageFileSystem.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __CMASSSTORAGEFILESYSTEM_H__
#define __CMASSSTORAGEFILESYSTEM_H__


//Forward Declaration
class CUsbMassStorageController;

/**
Mass Storage Filesystem class.
Only supports creating a new mount. Calling NewFileL, NewDirL and NewFormatL
results in the functions leaving with KErrNotReady.
When this file system is installed a new thread is created to load the
Usb Mass Storage controller.
@internalTechnology
*/
class CMassStorageFileSystem : public CFileSystem
	{
public:
	static CMassStorageFileSystem* NewL();
	~CMassStorageFileSystem();

	//CFileSystem
	TBool IsExtensionSupported() const;
	TInt Remove();
	TInt Install();
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
	TInt DefaultPath(TDes& aPath) const;
	TInt DriveList(TDriveList& aList) const;

	CUsbMassStorageController& Controller();
	TInt InitThread();
	TInt InitThreadL();

private:
	CMassStorageFileSystem();
	TInt EnumerateMsDrivesL();
	void ConstructL();

public:	
	// Public so the mount can see it
	RArray<TBusLocalDrive> iLocalDriveForMediaFlag;
	CArrayFixFlat<TBool> *iMediaChanged;
	TRequestStatus iThreadStat;
	TBool iInstalled;

private:
	CUsbMassStorageController* iMassStorageController;
	TBool iRunning;
	RArray<TInt> iMsDrives;
	};

#endif // __CMASSSTORAGEFILESYSTEM_H__

