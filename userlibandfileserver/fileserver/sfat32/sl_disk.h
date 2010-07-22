// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat\inc\sl_disk.h
// 
//

/**
 @file
 @internalTechnology
*/

#if !defined(__SL_DISK_H__)
#define __SL_DISK_H__

#include "sl_std.h"

//---------------------------------------------------------------------------------------------------------------------------------

class MWTCacheInterface;


class CAtaDisk : public CRawDisk
	{
public:
	static CAtaDisk* NewL(CFatMountCB& aFatMount);
	
     CAtaDisk(CFatMountCB& aFatMount);
	~CAtaDisk();

    void ConstructL();  
    void InitializeL();

public:
	void ReadCachedL(TInt64 aPos,TInt aLength,TDes8& aDes) const; 
	void WriteCachedL(TInt64 aPos,const TDesC8& aDes);

    void InvalidateUidCache();
    virtual void InvalidateUidCachePage(TUint64 aPos);

	
    void ReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const;
	void WriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag);
	virtual TInt GetLastErrorInfo(TDes8& aErrorInfo) const;

    MWTCacheInterface* DirCacheInterface();

    


private:

	TDriveInterface&    iDrive;     ///< Driver's interface to access the media
    MWTCacheInterface*  ipDirCache; ///< pointer to the FAT Directory cache object
    MWTCacheInterface*  iUidCache;  ///< pointer to the UID cache object

    };


//---------------------------------------------------------------------------------------------------------------------------------

class CRamDisk : public CRawDisk
	{
public:
	
    static CRamDisk* NewL(CFatMountCB& aFatMount);
	CRamDisk(CFatMountCB& aFatMount);

    void InitializeL();
public:
	void ReadCachedL(TInt64 aPos,TInt aLength,TDes8& aDes) const;
	void WriteCachedL(TInt64 aPos,const TDesC8& aDes);
	void ReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag) const;
	void WriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,const RMessagePtr2 &aMessage,TInt anOffset, TUint aFlag);
	
private:
	inline TUint8 *RamDiskBase() const;

private:
	TUint8* iRamDiskBase; ///< pointer to the beginning of the RAM disk memory area
	};


//---------------------------------------------------------------------------------------------------------------------------------

#include "sl_disk.inl"

#endif //__SL_DISK_H__














