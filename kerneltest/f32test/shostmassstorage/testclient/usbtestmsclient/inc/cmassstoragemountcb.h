// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Class declaration for CMassStorageMountCB.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef __CMASSSTORAGEMOUNTCB_H__
#define __CMASSSTORAGEMOUNTCB_H__


/**
Mass Storage Mount.
Only the MountL, Dismounted and Unlock methods are supported. All other methods
leave with KErrNotReady.
ControlIO is also supported for debug builds and returns KErrNotSupported for Release builds.
@internalTechnology
*/
class CMassStorageMountCB : public CLocDrvMountCB
	{
public:
	static CMassStorageMountCB* NewL(const TLunToDriveMap& aDriveMapping);
	void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	void VolumeL(TVolumeInfo& aVolume) const;
	void SetVolumeL(TDes& aName);
	void MkDirL(const TDesC& aName);
	void RmDirL(const TDesC& aName);
	void DeleteL(const TDesC& aName);
	void RenameL(const TDesC& anOldName,const TDesC& anNewName);
	void ReplaceL(const TDesC& anOldName,const TDesC& anNewName);
	void EntryL(const TDesC& aName,TEntry& anEntry) const;
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt anOffset, const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt anOffset, const RMessagePtr2& aMessage);
	void ReadSectionL(const TDesC& aName, TInt aPos, TAny* aTrg, TInt aLength, const RMessagePtr2& aMessage);
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShorName,TDes& aLongName);
	TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	TInt Unlock(TMediaPassword& aPassword, TBool aStore);

private:
	CMassStorageMountCB(const TLunToDriveMap& aDriveMapping);
	void WritePasswordData();
	TInt DriveNumberToLun(TInt aDriveNumber);
	TInt CheckDriveNumberL();

    TBool IsPowerOfTwo(TInt aNum);
    TInt Log2(TInt aNum);

private:
	const TLunToDriveMap& iDriveMapping;
	};


#endif //__CMASSSTORAGEMOUNTCB_H__
