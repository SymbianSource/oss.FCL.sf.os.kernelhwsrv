// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat\sl_fmt.cpp
// 
//

#include "sl_std.h"
#include <e32hal.h>




//
// Returns the total available ram from UserHal:: or sets an
// arbitrary limit upon the WINS ramdisk.
//
static TInt64 GetRamDiskSizeInBytes()
    {

#if defined(__EPOC32__)
    TMemoryInfoV1Buf memInfo;
    UserHal::MemoryInfo(memInfo);
    TUint max = memInfo().iTotalRamInBytes; // not really the correct max
    return max;
#else
    const TInt KArbitraryWinsRamDiskSize=0x400000;  //-- Default size for a Ram drive, 4MB
    return(KArbitraryWinsRamDiskSize);
#endif
    }

CFatFormatCB::CFatFormatCB()
    {
    __PRINT1(_L("CFatFormatCB::CFatFormatCB() [%x]"),this);
    }

CFatFormatCB::~CFatFormatCB()
    {
    __PRINT1(_L("CFatFormatCB::~CFatFormatCB() [%x]"),this);
    iBadSectors.Close();
    iBadClusters.Close();
    }

/**
    Calculate the size of a 16 bit FAT
*/
TUint CFatFormatCB::MaxFat16Sectors() const
    {
    const TUint32 fatSizeInBytes=(2*iMaxDiskSectors)/iSectorsPerCluster+(iBytesPerSector-1);
    return(fatSizeInBytes/iBytesPerSector);
    }


/**
    Calculate the size of a 12 bit FAT
*/
TUint CFatFormatCB::MaxFat12Sectors() const
    {
    const TUint32 maxDiskClusters=iMaxDiskSectors/iSectorsPerCluster;
    const TUint32 fatSizeInBytes=maxDiskClusters+(maxDiskClusters>>1)+(iBytesPerSector-1);
    
    return(fatSizeInBytes/iBytesPerSector);
    }

//-------------------------------------------------------------------------------------------------------------------
/**
    Fill a media range from aStartPos to aEndPos with zeroes.
    @param  aStartPos   start media position
    @param  aEndPos     end media position
*/
void CFatFormatCB::DoZeroFillMediaL(TInt64 aStartPos, TInt64 aEndPos)
    {
    ASSERT(aStartPos <= aEndPos && aStartPos >=0  && aEndPos >=0);

    RBuf8 buf;
    CleanupClosePushL(buf);

    const TInt KBufMaxSz=32768; //-- zero-buffer Maximal size, bytes
    const TInt KBufMinSz=512;   //-- zero-buffer minimal size, bytes

    if(buf.CreateMax(KBufMaxSz) != KErrNone)
        {
        buf.CreateMaxL(KBufMinSz); //-- OOM, try to create smaller buffer
        }

    buf.FillZ();

    TInt64 rem = aEndPos - aStartPos;
    while(rem)
        {
        const TUint32 bytesToWrite=(TUint32)Min(rem, buf.Size());
        TPtrC8 ptrData(buf.Ptr(), bytesToWrite);

        User::LeaveIfError(LocalDrive()->Write(aStartPos, ptrData));

        aStartPos+=bytesToWrite;
        rem-=bytesToWrite;
        }
    
    CleanupStack::PopAndDestroy(&buf); 
    }

//-------------------------------------------------------------------------------------------------------------------

static TUint32 DiskSizeInSectorsL(TInt64 aSizeInBytes)
    {
    const TInt64 totalSectors64=aSizeInBytes>>KDefSectorSzLog2;
    const TUint32   totalSectors32=I64LOW(totalSectors64);
    ASSERT(!I64HIGH(totalSectors64));
    //__PRINT2(_L("Disk size:%LU, max disk sectors:%d"),aSizeInBytes, totalSectors32);
    return totalSectors32;
    }

//-------------------------------------------------------------------------------------------------------------------
/**
    suggest FAT type according to the FAT volume metrics
    @return calculated FAT type
*/
TFatType CFatFormatCB::SuggestFatType() const
{
    const TUint32 rootDirSectors = (iRootDirEntries*KSizeOfFatDirEntry + (iBytesPerSector-1)) / iBytesPerSector;
    const TUint32 dataSectors = iMaxDiskSectors - (iReservedSectors + (iNumberOfFats * iSectorsPerFat) + rootDirSectors);
    const TUint32 clusterCnt = dataSectors/ iSectorsPerCluster;

    //-- magic. see FAT specs for details.
    if(clusterCnt < 4085)
        return EFat12;
    else if(clusterCnt < 65525)
        return EFat16;

    return EFat32;
}

//-------------------------------------------------------------------------------------------------------------------
void Dump_LocalDrvCaps(const TLocalDriveCapsV6& aCaps)
{
    (void)aCaps;
#ifdef _DEBUG
    __PRINT(_L("----- TLocalDriveCapsV6 dump:"));
    __PRINT1(_L("iSize:%LU"), aCaps.iSize);
    __PRINT4(_L("iType:%d, iDriveAtt:0x%x, iMediaAtt:0x%x, iConnectionBusType:%d"), aCaps.iType, aCaps.iDriveAtt, aCaps.iMediaAtt, aCaps.iConnectionBusType);
    __PRINT2(_L("iFileSystemId:%d, iPartitionType:%d"), aCaps.iFileSystemId, aCaps.iPartitionType);
    __PRINT2(_L("iHiddenSectors:%d, iEraseBlockSize:%d"), aCaps.iHiddenSectors, aCaps.iEraseBlockSize);
    __PRINT4(_L("iExtraInfo:%d, iNumberOfSectors:%d, iSectorSizeInBytes:%d, iBlockSize:%d"), aCaps.iExtraInfo, aCaps.iNumberOfSectors, aCaps.iSectorSizeInBytes, aCaps.iBlockSize);
    __PRINT(_L("-----"));
#endif
}


//-------------------------------------------------------------------------------------------------------------------
/**
    Initialize format data.
*/
void CFatFormatCB::InitializeFormatDataL()
    {
      
    __PRINT1(_L("CFatFormatCB::InitializeFormatDataL() drv:%d"), Drive().DriveNumber());
    
    TLocalDriveCapsV6Buf capsBuf;
    const TLocalDriveCapsV6& caps = capsBuf();
    User::LeaveIfError(LocalDrive()->Caps(capsBuf));

    Dump_LocalDrvCaps(caps);

    iVariableSize   = (caps.iMediaAtt & KMediaAttVariableSize);
    iBytesPerSector=KDefaultSectorSize;
    iSectorSizeLog2 = Log2(iBytesPerSector);
    iHiddenSectors  = caps.iHiddenSectors;  
    iNumberOfHeads=2;
    iSectorsPerTrack=16;
    
    TInt nRes=KErrUnknown;

    if (iVariableSize)
        {// Variable size implies ram disk
        iMaxDiskSectors=DiskSizeInSectorsL(GetRamDiskSizeInBytes());
        nRes = ProcessVolParam_RamDisk(caps);
        }
    else
        {//-- fixed-size media
        iMaxDiskSectors=DiskSizeInSectorsL(caps.iSize);
        __PRINT3(_L("::InitializeFormatDataL() iMode:0x%x, ilen:%d, extrai:%d"), iMode, iSpecialInfo.Length(), caps.iExtraInfo);

        //-----------------------------------------------------------------------------------------
        //-- find out if there are volume parameters specified by the user or media driver.

        //-- meaning: the user has specified its own settings for the volume parameters
        const TBool bUserFormat = (iMode & ESpecialFormat) && iSpecialInfo.Length();
    
        //-- meaning: the media driver has its own settings regarding the volume parameters
        const TBool bCustomFormat = caps.iExtraInfo; 

        if(bUserFormat && bCustomFormat)
            {
            nRes = KErrNotSupported; //-- conflict between user settings and media driver's
            }
            else
            {
            if(bUserFormat)
                nRes = ProcessVolParam_User(caps);
            else if(bCustomFormat)
                nRes = ProcessVolParam_Custom(caps);
            else
                nRes = ProcessVolParam_Default(caps); 
            }
        } //else(iVariableSize)
    
    if(nRes != KErrNone)
        {
        __PRINT1(_L(" ::InitializeFormatDataL() err:%d"), nRes);
        User::Leave(nRes);
        }

    }


TInt CFatFormatCB::HandleCorrupt(TInt aError)
//
// Handle disk corrupt during format. It needs media driver's support.
// Media driver should handle DLocalDrive::EGetLastErrorInfo request in
// its Request function, filling in proper error information.
// @see TErrorInfo
//
    {
    __PRINT2(_L("CFatFormatCB::HandleCorrupt(%d) drv:%d"), aError, Drive().DriveNumber());

    TPckgBuf<TErrorInfo> info;
    TInt r = LocalDrive()->GetLastErrorInfo(info);
    
    if(r != KErrNone)
        {
        __PRINT1(_L("....GetLastErrorInfo() err:%d"), r);
        }

    if (r == KErrNotSupported)
        return KErrCorrupt;
    else if (r != KErrNone)
        return r;

    __PRINT3(_L("....TErrorInfo iReasonCode:%d, iErrorPos:%LU, iOtherInfo:%d"), info().iReasonCode, info().iErrorPos, info().iOtherInfo);
    
    // if no error reported by GetLastErrorInfo(), return the original error
    if (info().iReasonCode == KErrNone)
        return aError;

    if (info().iReasonCode!=KErrNone && info().iReasonCode!=TErrorInfo::EBadSector)
        return info().iReasonCode;

    // First bad sector met
    TInt sectorsDone = (TInt)(info().iErrorPos >> iSectorSizeLog2);
    TInt badSector = iFormatInfo.i512ByteSectorsFormatted + sectorsDone;
    iBadSectors.Append(badSector);

    // Update format information
    iFormatInfo.i512ByteSectorsFormatted += sectorsDone+1;
    return KErrNone;
    }

void CFatFormatCB::TranslateL()
//
// Change bad cluster number to new value with regard to new format parameters
//
    {
    if (iDiskCorrupt || !(iMode & EQuickFormat))
        return;

    TInt size = 1 << FatMount().ClusterSizeLog2();
    TUint8* readBuf = new(ELeave) TUint8[size];
    TPtr8 readBufPtr(readBuf, size);
    RArray<TInt> newArray;
    TInt r = DoTranslate(readBufPtr, newArray);
    
    delete[] readBuf;
    readBuf = NULL;

    newArray.Close();
    User::LeaveIfError(r);
    }

#define calcSector(n) (n+oFirstFreeSector-nFirstFreeSector)
TInt CFatFormatCB::DoTranslate(TPtr8& aBuf, RArray<TInt>& aArray)
    {

    TInt r = KErrNone;

    // old format parameters
    TInt oFirstFreeSector = iOldFirstFreeSector;
    TInt oSectorsPerCluster = iOldSectorsPerCluster;
    // new format parameters
    TInt nFirstFreeSector = FatMount().iFirstFreeByte>>FatMount().SectorSizeLog2();
    TInt nSectorsPerCluster = FatMount().SectorsPerCluster();

    if (oFirstFreeSector==nFirstFreeSector && oSectorsPerCluster==nSectorsPerCluster)
        return r;

    TInt i;
    for (i=0; i<iBadClusters.Count(); ++i)
        {
        /*
        Cluster boundary may change due to format parameter change.
        Old: |-- ... --|----|----|----|----|----|----|----|
                       |<-          Data area           ->|
        New: |--- ... ---|------|------|------|------|------|
                         |<-           Data area          ->|
        */
        TInt begSector = calcSector((iBadClusters[i]-2)*oSectorsPerCluster);
        begSector = Max(begSector, nFirstFreeSector);
        TInt endSector = calcSector(((iBadClusters[i]-1)*oSectorsPerCluster)-1);
        endSector = Max(endSector, nFirstFreeSector);
        TInt begCluster = (begSector/iSectorsPerCluster)+KFatFirstSearchCluster;
        TInt endCluster = (endSector/iSectorsPerCluster)+KFatFirstSearchCluster;
        if (begCluster == endCluster)  // old cluster is in a new cluster
            {
            if (aArray.Find(begCluster) == KErrNotFound)
                if ((r=aArray.Append(begCluster)) != KErrNone)
                    return r;
            continue;
            }
        // deal with old cluster cross over several new clusters
        TInt offset = (begSector-(begCluster-2)*iSectorsPerCluster)<<iSectorSizeLog2;
        TInt len = (endSector-(endCluster-2)*iSectorsPerCluster)<<iSectorSizeLog2;
        TInt j;
        for (j=begCluster; j<=endCluster; ++j)
        // Because each old bad cluster cross several new clusters,
        // we have to verify which new cluster is bad really
            {
            TInt addr = (nFirstFreeSector+(j-2)*iSectorsPerCluster)<<iSectorSizeLog2;
            TInt clusterLen = (1<<iSectorSizeLog2) * iSectorsPerCluster;
            if (j == begCluster)
                r = LocalDrive()->Read(addr+offset,clusterLen-offset,aBuf);
            else if (j == endCluster && len)
                r = LocalDrive()->Read(addr,len,aBuf);
            else
                r = LocalDrive()->Read(addr,clusterLen,aBuf);
            if (r == KErrCorrupt) // new cluster j is corrupt
                if ((r=aArray.Append(j)) != KErrNone)
                    return r;
            }
        }
    // Update iBadClusters with aArray
    iBadClusters.Reset();
    for (i=0; i<aArray.Count(); ++i)
        if ((r=iBadClusters.Append(aArray[i])) != KErrNone)
            return r;
    iBadClusters.Sort();
    return r;
    }


//-------------------------------------------------------------------------------------------------------------------
/** override from CFormatCB, additional interfaces implementation */
TInt CFatFormatCB::GetInterface(TInt aInterfaceId, TAny*& /*aInterface*/, TAny* aInput)
    {
    if(aInterfaceId == ESetFmtParameters)
        {
        return DoProcessTVolFormatParam((const TVolFormatParam_FAT*)aInput);
        }

    return KErrNotSupported;
    }

//-------------------------------------------------------------------------------------------------------------------
/** 
    Process formatting parameters passed as TVolFormatParam_FAT structure.
    @param      apVolFormatParam pointer to the formatting parameters.
    @return     standard error code
*/
TInt CFatFormatCB::DoProcessTVolFormatParam(const TVolFormatParam_FAT* apVolFormatParam)
    {
    if(apVolFormatParam->iUId != TVolFormatParam::KUId ||  apVolFormatParam->FSNameHash() != TVolFormatParam::CalcFSNameHash(KFileSystemName_FAT))
        {
        ASSERT(0);
        return KErrArgument;
        }

    //-- Populate iSpecialInfo with the data taken from apVolFormatParam.
    //-- for formatting FAT volume iSpecialInfo can hold absolutely all required data from apVolFormatParam.
    //-- if some additional data from apVolFormatParam are required for some reason, figure out youself how to store and use them.
    TLDFormatInfo& fmtInfo = iSpecialInfo();
    new(&fmtInfo) TLDFormatInfo; //-- initialise the structure in the buffer 


    //-- sectors per cluster
    fmtInfo.iSectorsPerCluster = (TUint16)apVolFormatParam->SectPerCluster();   
    
    //-- FAT type
    const TFatSubType fatSubType = apVolFormatParam->FatSubType();
    
    if(fatSubType != ENotSpecified && fatSubType != EFat12 && fatSubType != EFat16 && fatSubType != EFat32)
        return KErrArgument;


    fmtInfo.iFATBits = (TLDFormatInfo::TFATBits)fatSubType; //-- FAT12/16/32/not specified

    //-- number of FAT tables
    switch(apVolFormatParam->NumFATs())
        {
        case 0: //-- "not specified, default"
        break;

        case 1:
            fmtInfo.iFlags |= TLDFormatInfo::EOneFatTable; 
        break;

        case 2:
            fmtInfo.iFlags |= TLDFormatInfo::ETwoFatTables; 
        break;

        default: //-- more than KMaxFatTablesSupported is not supported
        return KErrArgument;

        };

    //-- number of reserved sectors
    fmtInfo.iReservedSectors = (TUint16)apVolFormatParam->ReservedSectors();

    return KErrNone;
    }











