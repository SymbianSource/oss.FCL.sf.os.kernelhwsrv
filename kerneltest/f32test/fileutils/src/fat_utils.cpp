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
// @file
// various FAT test utilities 
// 
//


#include "fat_utils.h"
using namespace Fat_Test_Utils;



//-------------------------------------------------------------------------------------------------------------------

//-- define this macro if it is necessarily to exclude all stuff that uses RFs, consoles etc.
//-- may be useful if this code is used for file server extensions. mmp file is a good place to define this macro.
#ifndef FAT_UTILS_LEAN_AND_MEAN

#include <e32cons.h>
#include <e32math.h>

//-------------------------------------------------------------------------------------------------------------------

/**
    Format FAT volume
    
    @param  aFs             reference to the FS session
    @param  aDrive          drive number
    @param  aQuickFormat    if True, a quick format will be performed. otherwise - full
    @param  apFmtParams     pointer to the optional format parameters. if NULL, it means tha no options specified
    @param  aForceErase     if True, use media force erase.

    @return system-wide error codes.
*/
TInt Fat_Test_Utils::FormatFatDrive(RFs &aFs, TInt aDrive, TBool aQuickFormat, const TFatFormatParam* apFmtParams/*=NULL*/, TBool aForceErase /*=EFalse*/)
{
    TPtrC fmtTypeName = (aQuickFormat ? _L("Quick") : _L("Full"));
    DoPrintf(_L("~ Fat_Test_Utils::FormatFatDrive() drv:%d, type:%S\n"),aDrive, &fmtTypeName);
   
    ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    
    RFormat format;
    TUint   fmtMode=0;
    TInt    fmtCnt=0;
    TInt    prevCnt;
    TInt    nRes;

    if(aQuickFormat) 
        fmtMode |= EQuickFormat;

    if(aForceErase)
        fmtMode |= EForceErase;

    TBuf<10> drvName;
    drvName.Format(_L("%C:"),'A'+aDrive);

    if(!apFmtParams)
    {//-- no format parameters specified
        //DoPrintf(_L("~ Format parameters - not specified\n"));
        nRes = format.Open(aFs, drvName, fmtMode, fmtCnt);
        if(nRes!=KErrNone)
            goto Fail;
    }
    else
    {//-- some extra parameters specified, use special format
        fmtMode |= ESpecialFormat;
        TLDFormatInfo formatInfo;

        TBuf<100> buf;
        buf.Copy(_L("~ Format parameters: "));
        
        if(apFmtParams->iFatType)
        {//-- FAT type is specified
            buf.AppendFormat(_L("FAT%d"), apFmtParams->iFatType);
            formatInfo.iFATBits = (TLDFormatInfo::TFATBits)apFmtParams->iFatType;
        }

        const TUint16 spc = (TUint16)apFmtParams->iSecPerCluster;
        if(spc)
        {//-- sectors per cluster value is specified
            buf.AppendFormat(_L(", spc:%d"), spc);
            formatInfo.iSectorsPerCluster = spc;
        }

        const TUint16 rsvdSec = (TUint16)apFmtParams->iReservedSectors;
        if(rsvdSec)
        {//-- reserved sectors numer is specified
            buf.AppendFormat(_L(", rsvdSec:%d"), rsvdSec);
            formatInfo.iReservedSectors = rsvdSec;
        }
        
        buf.Append(_L("\n"));
        DoPrintf(buf);
        
        TSpecialFormatInfoBuf formatInfoBuf(formatInfo);
        nRes = format.Open(aFs, drvName, fmtMode, fmtCnt, formatInfoBuf);
        if(nRes!=KErrNone)
            goto Fail;

    }

    //-- do format steps
    prevCnt=fmtCnt;
    while(fmtCnt)
    {
        nRes = format.Next(fmtCnt);
        if(nRes!=KErrNone)
            goto Fail;

        if(fmtCnt != prevCnt)
        {
            DoPrintf(_L("."));
            prevCnt = fmtCnt;
        }
    }

    //-- formatting has finished
    DoPrintf(_L("\n"));
    format.Close();
    return KErrNone;

   Fail:
    format.Close();
    DoPrintf(_L("~ Fat_Test_Utils::FormatFatDrive() failed! code:%d\n"), nRes);

    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Read FAT FSInfo sector via RRawDisk interface.
    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aMediaPos   media position (0 for the main boot sector)
    @param  aFsInfo     where to read data

    @return standard error codes
*/
TInt Fat_Test_Utils::ReadFSInfoSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TFSInfo& aFsInfo)
{
    DoPrintf(_L("~ Fat_Test_Utils::ReadFSInfoSector() drv:%d, pos:0x%x\n"),aDrive, (TUint32)aMediaPos);   

    TInt nRes = KErrNone;
    
    TBuf8<KSizeOfFSInfo> fsInfoSecBuf(KSizeOfFSInfo);
    TRAP(nRes, DoMediaRawReadL(aFs, aDrive, aMediaPos, fsInfoSecBuf.Size(), fsInfoSecBuf));

    if(nRes == KErrNone)
    {//-- take FSInfo data from the buffer
        aFsInfo.Internalize(fsInfoSecBuf);
    }

    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Write FAT FSInfo sector via RRawDisk interface.
    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aMediaPos   media position (0 for the main boot sector)
    @param  aFsInfo     data to write

    @return standard error codes
*/
TInt Fat_Test_Utils::WriteFSInfoSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TFSInfo& aFsInfo)
{
    DoPrintf(_L("~ Fat_Test_Utils::WriteFSInfoSector() drv:%d, pos:0x%x\n"),aDrive, (TUint32)aMediaPos);   
    
    TInt nRes = KErrNone;
    TBuf8<KSizeOfFSInfo> fsInfoSecBuf;

    //-- put data to the sector buffer
    aFsInfo.Externalize(fsInfoSecBuf);
  
    TRAP(nRes, DoMediaRawWriteL(aFs, aDrive, aMediaPos, fsInfoSecBuf))

    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Read FAT boot sector via RRawDisk interface.
    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aMediaPos   media position (0 for the main boot sector)
    @param  aBootSector where to read data

    @return standard error codes
*/
TInt  Fat_Test_Utils::ReadBootSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TFatBootSector& aBootSector)
{
    DoPrintf(_L("~ Fat_Test_Utils::ReadBootSector() drv:%d, pos:0x%x\n"),aDrive, (TUint32)aMediaPos);   
    
    TInt nRes = KErrNone;
    TBuf8<KSizeOfFatBootSector> bootSecBuf(KSizeOfFatBootSector);

    TRAP(nRes, DoMediaRawReadL(aFs, aDrive, aMediaPos, bootSecBuf.Size(), bootSecBuf));

    if(nRes==KErrNone)
    {//-- initialise TFatBootSector object
        aBootSector.Internalize(bootSecBuf);
    }

    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Write FAT boot sector via RRawDisk interface.
    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aMediaPos   media position (0 for the main boot sector)
    @param  aBootSector data to write

    @return standard error codes
*/
TInt  Fat_Test_Utils::WriteBootSector(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TFatBootSector& aBootSector)
{
    DoPrintf(_L("~ Fat_Test_Utils::WriteBootSector() drv:%d, pos:0x%x\n"),aDrive, (TUint32)aMediaPos);   
    
    TBuf8<KDefaultSectorSize> bootSecBuf(KDefaultSectorSize);
    
    //-- externalize boot sector to the data buffer
    bootSecBuf.FillZ();
    aBootSector.Externalize(bootSecBuf); 
    
    //-- put a boot sector signature to the last 2 bytes
    bootSecBuf[KDefaultSectorSize-2] = 0x55;
    bootSecBuf[KDefaultSectorSize-1] = 0xaa;

    //-- write boot sector data to the media
    TInt nRes=KErrNone;
    TRAP(nRes, DoMediaRawWriteL(aFs, aDrive, aMediaPos, bootSecBuf));
    
    return nRes;
}
//-------------------------------------------------------------------------------------------------------------------
/** 
    Obtain FAT type for the given drive 

    @return TFatType enum values. if aDrive has not FAT FS, EInvalid value is returned
*/
TFatType Fat_Test_Utils::GetFatType(RFs &aFs, TInt aDrive)
{
    if(Is_Fat16(aFs, aDrive))
        return EFat16;
    else if(Is_Fat32(aFs, aDrive))
        return EFat32;
    else if(Is_Fat12(aFs, aDrive))
        return EFat12;

    return EInvalid;
}

//-------------------------------------------------------------------------------------------------------------------

#endif //FAT_UTILS_LEAN_AND_MEAN




TFatFormatParam::TFatFormatParam()
{
    iFatType = EInvalid;
    iSecPerCluster = 0;
    iReservedSectors = 0;
}
        
//-------------------------------------------------------------------------------------------------------------------

TFatBootSector::TFatBootSector()
{
    Initialise();
}

/** initialises the boot sector data */
void TFatBootSector::Initialise()
{
    Mem::FillZ(this, sizeof(TFatBootSector));
}

TBool TFatBootSector::operator==(const TFatBootSector& aRhs)
{
    ASSERT(&aRhs != this);
    if(&aRhs == this)
        return ETrue; //-- comparing with itself

    return  (Mem::Compare((TUint8*)this, sizeof(TFatBootSector), (TUint8*)&aRhs, sizeof(TFatBootSector)) == 0);
}


/**
    @return ETrue if the boot sector contents seems to be valid
*/
TBool TFatBootSector::IsValid() const
{
    const TFatType fatType = FatType(); //-- it will check SectorsPerCluster etc.

    if(fatType == EInvalid || ReservedSectors() < 1 || NumberOfFats() < 1)
        goto Invalid;
        
    if(fatType == EFat32)
    {
        if(VersionNumber()!= 0 || FatSectors()!=0 || FatSectors32()<1 || RootClusterNum()<(TUint32)KFatFirstSearchCluser ||
           TotalSectors()!=0 || HugeSectors() <5 || RootDirEntries() !=0)
        {
            goto Invalid; //-- these values are not compliant with FAT specs
        }
    }
    else //-- FAT12/16
    {
        if(TotalSectors() >0 && HugeSectors() >0 )
            goto Invalid; //-- values clash

        const TUint32 totSectors = Max(TotalSectors(), HugeSectors());
        const TUint32 rootDirStartSec =  ReservedSectors() + FatSectors()*NumberOfFats(); //-- root directory start sector

        if(FatSectors() < 1 || rootDirStartSec < 3 || RootDirEntries() < 1 || totSectors < 5)
            goto Invalid; //-- these values are not compliant with FAT specs
    }

    return ETrue;
  
  Invalid:
    DoPrintf(_L("~ TFatBootSector::IsValid() failed!\n"));

    return EFalse;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Initialize boot sector object from the given bufer. Does not validate the data.
    @param  aBuf buffer with data.
*/
void TFatBootSector::Internalize(const TDesC8& aBuf)
{
    ASSERT(aBuf.Size() >= KSizeOfFatBootSector);

    Initialise();

    TInt pos=0;

    Mem::Copy(&iJumpInstruction, &aBuf[pos],3);     pos+=3; // 0    TUint8 iJumpInstruction[3]
    Mem::Copy(&iVendorId,&aBuf[pos],KVendorIdSize); pos+=KVendorIdSize; // 3    TUint8 iVendorId[KVendorIdSize]
    Mem::Copy(&iBytesPerSector,&aBuf[pos],2);       pos+=2; // 11   TUint16 iBytesPerSector
    Mem::Copy(&iSectorsPerCluster,&aBuf[pos],1);    pos+=1; // 13   TUint8 iSectorsPerCluster   
    Mem::Copy(&iReservedSectors,&aBuf[pos],2);      pos+=2; // 14   TUint16 iReservedSectors
    Mem::Copy(&iNumberOfFats,&aBuf[pos],1);         pos+=1; // 16   TUint8 iNumberOfFats
    Mem::Copy(&iRootDirEntries,&aBuf[pos],2);       pos+=2; // 17   TUint16 iRootDirEntries
    Mem::Copy(&iTotalSectors,&aBuf[pos],2);         pos+=2; // 19   TUint16 iTotalSectors
    Mem::Copy(&iMediaDescriptor,&aBuf[pos],1);      pos+=1; // 21   TUint8 iMediaDescriptor
    Mem::Copy(&iFatSectors,&aBuf[pos],2);           pos+=2; // 22   TUint16 iFatSectors
    Mem::Copy(&iSectorsPerTrack,&aBuf[pos],2);      pos+=2; // 24   TUint16 iSectorsPerTrack
    Mem::Copy(&iNumberOfHeads,&aBuf[pos],2);        pos+=2; // 26   TUint16 iNumberOfHeads
    Mem::Copy(&iHiddenSectors,&aBuf[pos],4);        pos+=4; // 28   TUint32 iHiddenSectors
    Mem::Copy(&iHugeSectors,&aBuf[pos],4);          pos+=4; // 32   TUint32 iHugeSectors

    if(RootDirEntries() == 0) //-- we have FAT32 volume
    {
        Mem::Copy(&iFatSectors32, &aBuf[pos],4);    pos+=4; // 36 TUint32 iFatSectors32     
        Mem::Copy(&iFATFlags, &aBuf[pos],2);        pos+=2; // 40 TUint16 iFATFlags
        Mem::Copy(&iVersionNumber, &aBuf[pos],2);   pos+=2; // 42 TUint16 iVersionNumber
        Mem::Copy(&iRootClusterNum, &aBuf[pos],4);  pos+=4; // 44 TUint32 iRootClusterNum
        Mem::Copy(&iFSInfoSectorNum, &aBuf[pos],2); pos+=2; // 48 TUint16 iFSInfoSectorNum
        Mem::Copy(&iBkBootRecSector, &aBuf[pos],2);         // 50 TUint16 iBkBootRecSector
        pos+=(2+12);    //extra 12 for the reserved bytes   
    }

    Mem::Copy(&iPhysicalDriveNumber,&aBuf[pos],1);  pos+=1;// 36|64 TUint8 iPhysicalDriveNumber
    Mem::Copy(&iReserved,&aBuf[pos],1);             pos+=1;// 37|65 TUint8 iReserved
    Mem::Copy(&iExtendedBootSignature,&aBuf[pos],1);pos+=1;// 38|66 TUint8 iExtendedBootSignature
    Mem::Copy(&iUniqueID,&aBuf[pos],4);             pos+=4;// 39|67 TUint32 iUniqueID
    Mem::Copy(&iVolumeLabel,&aBuf[pos],KVolumeLabelSize);  // 43|71 TUint8 iVolumeLabel[KVolumeLabelSize]
    pos+=KVolumeLabelSize;

    // 54|82    TUint8 iFileSysType[KFileSysTypeSize]
    ASSERT(aBuf.Size() >= pos+KFileSysTypeSize);
    Mem::Copy(&iFileSysType,&aBuf[pos],KFileSysTypeSize);
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Externalize boot sector object to the given data buffer.
    @param  aBuf buffer to externalize.
*/
void TFatBootSector::Externalize(TDes8& aBuf) const
{
    ASSERT(aBuf.MaxSize() >= KSizeOfFatBootSector);

    if(aBuf.Size() < KSizeOfFatBootSector)
        aBuf.SetLength(KSizeOfFatBootSector);
    
    TInt pos=0;

    Mem::Copy(&aBuf[pos],&iJumpInstruction,3);      pos+=3;
    Mem::Copy(&aBuf[pos],&iVendorId,KVendorIdSize); pos+=8;
    Mem::Copy(&aBuf[pos],&iBytesPerSector,2);       pos+=2;
    Mem::Copy(&aBuf[pos],&iSectorsPerCluster,1);    pos+=1;
    Mem::Copy(&aBuf[pos],&iReservedSectors,2);      pos+=2;
    Mem::Copy(&aBuf[pos],&iNumberOfFats,1);         pos+=1;
    Mem::Copy(&aBuf[pos],&iRootDirEntries,2);       pos+=2;
    Mem::Copy(&aBuf[pos],&iTotalSectors,2);         pos+=2;
    Mem::Copy(&aBuf[pos],&iMediaDescriptor,1);      pos+=1;
    Mem::Copy(&aBuf[pos],&iFatSectors,2);           pos+=2;
    Mem::Copy(&aBuf[pos],&iSectorsPerTrack,2);      pos+=2;
    Mem::Copy(&aBuf[pos],&iNumberOfHeads,2);        pos+=2;
    Mem::Copy(&aBuf[pos],&iHiddenSectors,4);        pos+=4;
    Mem::Copy(&aBuf[pos],&iHugeSectors,4);          pos+=4;

    if(iFatSectors == 0)    
        {
        Mem::Copy(&aBuf[pos], &iFatSectors32,4);    pos+=4;
        Mem::Copy(&aBuf[pos], &iFATFlags, 2);       pos+=2;
        Mem::Copy(&aBuf[pos], &iVersionNumber, 2);  pos+=2;
        Mem::Copy(&aBuf[pos], &iRootClusterNum, 4); pos+=4;
        Mem::Copy(&aBuf[pos], &iFSInfoSectorNum, 2);pos+=2;
        Mem::Copy(&aBuf[pos], &iBkBootRecSector, 2);pos+=2;

        //extra 12 for the reserved bytes   
        ASSERT(aBuf.Size() >= pos+12);
        Mem::FillZ(&aBuf[pos],12);
        pos+=12;
        }

    Mem::Copy(&aBuf[pos],&iPhysicalDriveNumber,1);  pos+=1;
    Mem::FillZ(&aBuf[pos],1);                       pos+=1;
    Mem::Copy(&aBuf[pos],&iExtendedBootSignature,1);pos+=1;
    Mem::Copy(&aBuf[pos],&iUniqueID,4);             pos+=4;
    
    Mem::Copy(&aBuf[pos],&iVolumeLabel,KVolumeLabelSize); 
    pos+=KVolumeLabelSize;
    
    ASSERT(aBuf.MaxSize() >= pos+KFileSysTypeSize);
    Mem::Copy(&aBuf[pos],&iFileSysType,KFileSysTypeSize);
}

//-------------------------------------------------------------------------------------------------------------------

/** replaces all non-printable characters in a buffer with spaces */
static void FixDes(TDes& aDes)
{
    for(TInt i=0; i< aDes.Length(); ++i)
    {
        TChar ch=aDes[i];
        if(!ch.IsPrint())
            aDes[i]=' ';    
    }
}

/** 
    Print out the boot sector info.
*/
void TFatBootSector::PrintDebugInfo() const
{
    TBuf<40> buf;
    
    DoPrintf(_L("======== BootSector info: =======\n"));
    
    buf.Copy(FileSysType()); FixDes(buf);    
    DoPrintf(_L("FAT type:%S\n"),&buf);

    buf.Copy(VendorId()); FixDes(buf);
    DoPrintf(_L("Vendor ID:%S\n"),&buf);

    DoPrintf(_L("BytesPerSector: %d\n"),BytesPerSector());
    DoPrintf(_L("SectorsPerCluster: %d\n"),SectorsPerCluster());
    DoPrintf(_L("ReservedSectors: %d\n"),ReservedSectors());
    DoPrintf(_L("NumberOfFats: %d\n"),NumberOfFats());
    DoPrintf(_L("RootDirEntries: %d\n"),RootDirEntries());
    DoPrintf(_L("Total Sectors: %d\n"),TotalSectors());
    DoPrintf(_L("MediaDescriptor: 0x%x\n"),MediaDescriptor());
    DoPrintf(_L("FatSectors: %d\n"),FatSectors());
    DoPrintf(_L("SectorsPerTrack: %d\n"),SectorsPerTrack());
    DoPrintf(_L("NumberOfHeads: %d\n"),NumberOfHeads());
    DoPrintf(_L("HugeSectors: %d\n"),HugeSectors());
    DoPrintf(_L("Fat32 Sectors: %d\n"),FatSectors32());
    DoPrintf(_L("Fat32 Flags: %d\n"),FATFlags());
    DoPrintf(_L("Fat32 Version Number: %d\n"),VersionNumber());
    DoPrintf(_L("Root Cluster Number: %d\n"),RootClusterNum());
    DoPrintf(_L("FSInfo Sector Number: %d\n"),FSInfoSectorNum());
    DoPrintf(_L("Backup Boot Rec Sector Number: %d\n"),BkBootRecSector());
    DoPrintf(_L("PhysicalDriveNumber:%d\n"),PhysicalDriveNumber());
    DoPrintf(_L("ExtendedBootSignature: %d\n"),ExtendedBootSignature());
    DoPrintf(_L("UniqueID: 0x%x\n"),UniqueID());
    
    buf.Copy(VolumeLabel()); FixDes(buf);
    DoPrintf(_L("VolumeLabel:%S\n"),&buf);
    
    DoPrintf(_L("=============================\n"));
}

//-------------------------------------------------------------------------------------------------------------------


/**
    Determine FAT type according to the information from boot sector, see FAT32 specs.
    @return  FAT type. 
*/
TFatType TFatBootSector::FatType(void) const
    {

    //-- check iBytesPerSector validity; it shall be one of: 512,1024,2048,4096
    if(!IsPowerOf2(iBytesPerSector) || iBytesPerSector < 512 ||  iBytesPerSector > 4096)
        return EInvalid; //-- invalid iBytesPerSector value

    //-- check iSectorsPerCluster validity, it shall be one of: 1,2,4,8...128
    if(!IsPowerOf2(iSectorsPerCluster) || iSectorsPerCluster > 128)
        return EInvalid; //-- invalid iSectorsPerCluster value

    const TUint32 rootDirSectors = (iRootDirEntries*KSizeOfFatDirEntry + (iBytesPerSector-1)) / iBytesPerSector;
    const TUint32 fatSz = iFatSectors ? iFatSectors : iFatSectors32;
    const TUint32 totSec = iTotalSectors ? iTotalSectors : iHugeSectors;
    const TUint32 dataSec = totSec - (iReservedSectors + (iNumberOfFats * fatSz) + rootDirSectors);
    const TUint32 clusterCnt = dataSec / iSectorsPerCluster;

    //-- magic. see FAT specs for details.
    if(clusterCnt < 4085)
        return EFat12;
    else if(clusterCnt < 65525)
        return EFat16;
    else
        return EFat32;

    }

//-------------------------------------------------------------------------------------------------------------------

/** Returns Sectors in Fat table for 32 bit volume */
TUint32 TFatBootSector::FatSectors32() const	
{return iFatSectors32;}

/** Fat flags */
TUint16 TFatBootSector::FATFlags() const		
{return iFATFlags;}

/** Version number of the file system */
TUint16 TFatBootSector::VersionNumber() const		
{return iVersionNumber;}

/** Cluster number of the root directory */
TUint32 TFatBootSector::RootClusterNum() const	
{return iRootClusterNum;}

/** Sector number containing the FSIInfo structure */
TUint16 TFatBootSector::FSInfoSectorNum() const
{return iFSInfoSectorNum;}

/** Backup boot sector */
TUint16 TFatBootSector::BkBootRecSector() const
{return iBkBootRecSector;}

/** Sets the number of sectors in Fat table for 32 bit volume */
void TFatBootSector::SetFatSectors32(TUint32	aFatSectors32)
{iFatSectors32 = aFatSectors32;}

/** Sets the Fat flags */
void TFatBootSector::SetFATFlags(TUint16 aFATFlags)
{iFATFlags = aFATFlags;}

/** Sets the version number of the file system */
void TFatBootSector::SetVersionNumber(TUint16 aVersionNumber)
{iVersionNumber = aVersionNumber;}

/** Sets the cluster number of the root directory */
void TFatBootSector::SetRootClusterNum(TUint32 aRootClusterNum)	
{iRootClusterNum = aRootClusterNum;}

/** Set the sector number containing the FSIInfo structure */
void TFatBootSector::SetFSInfoSectorNum(TUint16 aFSInfoSectorNum)
{iFSInfoSectorNum = aFSInfoSectorNum;}

/** Set the backup boot sector */
void TFatBootSector::SetBkBootRecSector(TUint16 aBkBootRecSector)
{iBkBootRecSector = aBkBootRecSector;}	

/** Returns the vendor ID of the file system that formatted the volume */
const TPtrC8 TFatBootSector::VendorId() const
{return TPtrC8(iVendorId,KVendorIdSize);}

/** Return the bytes per sector */
TUint16 TFatBootSector::BytesPerSector() const
{return iBytesPerSector;}

/** Returns the sectors per cluster ratio */
TInt TFatBootSector::SectorsPerCluster() const
{return iSectorsPerCluster;}

/** Returns the number of reserved sectors on the volume */
TInt TFatBootSector::ReservedSectors() const
{return iReservedSectors;}

/** Returns the number of Fats on the volume */
TInt TFatBootSector::NumberOfFats() const
{return iNumberOfFats;}

/** Returns the number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32 */
TInt TFatBootSector::RootDirEntries() const
{return iRootDirEntries;}

/** Returns the total sectors on the volume, zero for FAT32 */
TInt TFatBootSector::TotalSectors() const
{return iTotalSectors;}

/** Returns the media descriptor */
TUint8 TFatBootSector::MediaDescriptor() const
{return iMediaDescriptor;}

/** Returns sectors used for the Fat table, zero for FAT32 */
TInt TFatBootSector::FatSectors() const
{return iFatSectors;}

/** Returns sectors per track */
TInt TFatBootSector::SectorsPerTrack() const
{return iSectorsPerTrack;}

/** Returns the number of heads  */
TInt TFatBootSector::NumberOfHeads() const
{return iNumberOfHeads;}

/** Returns the number of hidden sectors in the volume */
TInt TFatBootSector::HiddenSectors() const
{return iHiddenSectors;}

/** Returns total sectors in the volume, Used if totalSectors > 65535 */
TInt TFatBootSector::HugeSectors() const
{return iHugeSectors;}

/** Returns the physical drive number, not used in Symbian OS */
TInt TFatBootSector::PhysicalDriveNumber() const
{return iPhysicalDriveNumber;}

/** Returns the extended boot signiture */
TInt TFatBootSector::ExtendedBootSignature() const
{return iExtendedBootSignature;}

/** Returns the unique volume ID */
TUint32 TFatBootSector::UniqueID() const
{return iUniqueID;}

/** Returns the volume's label */
const TPtrC8 TFatBootSector::VolumeLabel() const
{return TPtrC8(iVolumeLabel,KVolumeLabelSize);}

/** Returns the file system type */
const TPtrC8 TFatBootSector::FileSysType() const
{return TPtrC8(iFileSysType,KFileSysTypeSize);}

/** Returns the boot sector signiture */
TInt TFatBootSector::BootSectorSignature() const
{return KBootSectorSignature;}

/** Set the jump instruction  */
void TFatBootSector::SetJumpInstruction()
{iJumpInstruction[0]=0xE9;iJumpInstruction[2]=0x90;}

/** Set the vendor ID of the file system that formatted the volume */
void TFatBootSector::SetVendorID(const TDesC8& aDes)
{
	ASSERT(aDes.Length()<=KVendorIdSize);
	TPtr8 buf(iVendorId,KVendorIdSize);
	buf=aDes;
}

/** Sets the bytes per sector  */
void TFatBootSector::SetBytesPerSector(TInt aBytesPerSector)
{
	ASSERT(!(aBytesPerSector&~KMaxTUint16));
	iBytesPerSector=(TUint16)aBytesPerSector;
}

/** Set the sectors per cluster ratio */
void TFatBootSector::SetSectorsPerCluster(TInt aSectorsPerCluster)
{
	ASSERT(!(aSectorsPerCluster&~KMaxTUint8));
	iSectorsPerCluster=(TUint8)aSectorsPerCluster;
}


/** Sets the number of reserved sectors on the volume */
void TFatBootSector::SetReservedSectors(TInt aReservedSectors)
{
	ASSERT(!(aReservedSectors&~KMaxTUint16));
	iReservedSectors=(TUint16)aReservedSectors;
}

/** Sets the number of Fats on the volume */
void TFatBootSector::SetNumberOfFats(TInt aNumberOfFats)
{
	ASSERT(!(aNumberOfFats&~KMaxTUint8));
	iNumberOfFats=(TUint8)aNumberOfFats;
}

/** Number of entries allowed in the root directory, specific to Fat12/16, zero for FAT32 */
void TFatBootSector::SetRootDirEntries(TInt aRootDirEntries)
{
	ASSERT(!(aRootDirEntries&~KMaxTUint16));
	iRootDirEntries=(TUint16)aRootDirEntries;
}

/** Total sectors on the volume, zero for FAT32 */
void TFatBootSector::SetTotalSectors(TInt aTotalSectors)
{
	ASSERT(!(aTotalSectors&~KMaxTUint16));
	iTotalSectors=(TUint16)aTotalSectors;
}

/** Set the media descriptor */
void TFatBootSector::SetMediaDescriptor(TUint8 aMediaDescriptor)
{iMediaDescriptor=aMediaDescriptor;}

/** Sectors used for the Fat table, zero for FAT32 */
void TFatBootSector::SetFatSectors(TInt aFatSectors)
{
	ASSERT(!(aFatSectors&~KMaxTUint16));
	iFatSectors=(TUint16)aFatSectors;
}

/** Set the sectors per track */
void TFatBootSector::SetSectorsPerTrack(TInt aSectorsPerTrack)
{
	ASSERT(!(aSectorsPerTrack&~KMaxTUint16));
	iSectorsPerTrack=(TUint16)aSectorsPerTrack;
}

/** Set the number of heads */
void TFatBootSector::SetNumberOfHeads(TInt aNumberOfHeads)
{
	ASSERT(!(aNumberOfHeads&~KMaxTUint16));
	iNumberOfHeads=(TUint16)aNumberOfHeads;
}

/** Set the number of hidden sectors in the volume */
void TFatBootSector::SetHiddenSectors(TUint32 aHiddenSectors)
{
	iHiddenSectors=(TUint32)(aHiddenSectors);
}

/** Set the total sectors in the volume, Used if totalSectors > 65535 */
void TFatBootSector::SetHugeSectors(TUint32 aHugeSectors)
{iHugeSectors=aHugeSectors;}


/** Physical drive number, not used in Symbian OS */
void TFatBootSector::SetPhysicalDriveNumber(TInt aPhysicalDriveNumber)
{
	ASSERT(!(aPhysicalDriveNumber&~KMaxTUint8));
	iPhysicalDriveNumber=(TUint8)aPhysicalDriveNumber;
}

/** Set the reserved byte value */
void TFatBootSector::SetReservedByte(TUint8 aReservedByte)
{iReserved=aReservedByte;}

/** Set the extended boot signiture */
void TFatBootSector::SetExtendedBootSignature(TInt anExtendedBootSignature)
{
	ASSERT(!(anExtendedBootSignature&~KMaxTUint8));
	iExtendedBootSignature=(TUint8)anExtendedBootSignature;
}

/** Set the unique volume ID */
void TFatBootSector::SetUniqueID(TUint32 anUniqueID)
{iUniqueID=anUniqueID;}

/** Set the volume's label */
void TFatBootSector::SetVolumeLabel(const TDesC8& aDes)
{
	ASSERT(aDes.Length()<=KVolumeLabelSize);
	TPtr8 buf(iVolumeLabel,KVolumeLabelSize);
	buf=aDes;
}

/** Set the file system type */
void TFatBootSector::SetFileSysType(const TDesC8& aDes)
{
	ASSERT(aDes.Length()<=8);
	TPtr8 buf(iFileSysType,8);
	buf=aDes;
}


/** @return The first Fat sector number */
TInt TFatBootSector::FirstFatSector() const
{
    __ASSERT_ALWAYS(IsValid(), User::Invariant());
    return ReservedSectors();
}

/**
    @return Number of sectors in root directory. 0 for FAT32
*/
TUint32 TFatBootSector::RootDirSectors() const
{
    __ASSERT_ALWAYS(IsValid(), User::Invariant());
    return ( (RootDirEntries()*KSizeOfFatDirEntry + (BytesPerSector()-1)) / BytesPerSector() );
}


/** @return Start sector number of the root directory */
TInt TFatBootSector::RootDirStartSector()  const
{
    __ASSERT_ALWAYS(IsValid(), User::Invariant());
    
    const TUint32 firstNonFatSec = ReservedSectors() + TotalFatSectors()*NumberOfFats();

    if(FatType() == EFat32)
    {//-- FAT32 root dir is a file, calculate the position by it's 1st cluster number. FAT[0]+FAT[1] are reserved.
        return (firstNonFatSec + (RootClusterNum()-KFatFirstSearchCluser) * SectorsPerCluster());
    }
    else
    {//-- FAT12/16 root dir starts just after the FATs
        return firstNonFatSec;
    }

}

/** @return first data sector number. for FAT32 it includes the root directory */
TInt TFatBootSector::FirstDataSector() const
{
    return( ReservedSectors() + NumberOfFats()*TotalFatSectors() + RootDirSectors() );
}

/** @return FAT-type independent sector count on the volume */
TUint32 TFatBootSector::VolumeTotalSectorNumber() const
{
    __ASSERT_ALWAYS(IsValid(), User::Invariant());
    return HugeSectors() ? (TUint32)HugeSectors() : (TUint32)TotalSectors();
}

/** @return FAT-type independent number of sectors in one FAT */
TUint32 TFatBootSector::TotalFatSectors() const
{
    __ASSERT_ALWAYS(IsValid(), User::Invariant());
    return FatSectors32() ? FatSectors32() : (TUint32)FatSectors();
}


//-------------------------------------------------------------------------------------------------------------------


const TUint32   KLeadSignature      = 0x41615252; ///< FSInfo Lead signiture value
const TUint32   KStructureSignature = 0x61417272; ///< FSInfo Structure signiture value
const TUint32   KTrailingSignature  = 0xAA550000; ///< FSInfo Trailing signiture

TFSInfo::TFSInfo()
{
    Initialise();
}
//-------------------------------------------------------------------------------------------------------------------

/** Initialise the data */
void TFSInfo::Initialise()  
{
    Mem::FillZ(this, sizeof(TFSInfo));

    iLeadSig      = KLeadSignature; 
    iStructureSig = KStructureSignature;
    iTrainlingSig = KTrailingSignature;
}


TBool TFSInfo::operator==(const TFSInfo& aRhs)
{
    ASSERT(&aRhs != this);
    if(&aRhs == this)
        return ETrue; //-- comparing with itself

    return  (Mem::Compare((TUint8*)this, sizeof(TFSInfo), (TUint8*)&aRhs, sizeof(TFSInfo)) == 0);
}

//-------------------------------------------------------------------------------------------------------------------

/**
    @return ETrue if FSInfo sector contents seems to be valid
*/
TBool TFSInfo::IsValid() const
{
    return (iLeadSig == KLeadSignature && iStructureSig == KStructureSignature && iTrainlingSig == KTrailingSignature);
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Initialize FSInfo sector object from the given bufer. Does not validate the data.
    @param  aBuf buffer with data.
*/
void TFSInfo::Internalize(const TDesC8& aBuf)
{
    ASSERT((TUint32)aBuf.Size() >= KSizeOfFSInfo);

    TInt pos=0;

    Mem::Copy(&iLeadSig, &aBuf[pos],4);      pos+=(KFSInfoReserved1Size+4);
    Mem::Copy(&iStructureSig, &aBuf[pos],4); pos+=4;
    Mem::Copy(&iFreeCount,&aBuf[pos],4);     pos+=4;
    Mem::Copy(&iNextFree,&aBuf[pos],4);      pos+=(4+KFSInfoReserved2Size);
    Mem::Copy(&iTrainlingSig,&aBuf[pos],4);
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Externalize FSInfo sector object to the given data buffer.
    @param  aBuf buffer to externalize.
*/
void TFSInfo::Externalize(TDes8& aBuf) const
{
    ASSERT((TUint32)aBuf.MaxSize() >= KSizeOfFSInfo);
    
    aBuf.SetLength(KSizeOfFSInfo);
    aBuf.FillZ();
    
    TInt pos=0;

    Mem::Copy(&aBuf[pos],&KLeadSignature,4);        pos+=4; 
                                                    pos+=KFSInfoReserved1Size;
    Mem::Copy(&aBuf[pos],&KStructureSignature,4);   pos+=4;
    Mem::Copy(&aBuf[pos],&iFreeCount,4);            pos+=4;
    Mem::Copy(&aBuf[pos],&iNextFree,4);             pos+=4;
                                                    pos+=KFSInfoReserved2Size;
    Mem::Copy(&aBuf[pos],&KTrailingSignature,4);
}

//-------------------------------------------------------------------------------------------------------------------

/** 
    Print out the FSInfo sector info.
*/
void TFSInfo::PrintDebugInfo() const
{
    DoPrintf(_L("==== FSInfoSector : ====\n"));
    DoPrintf(_L("FSI_LeadSig:   0x%x\n"),iLeadSig);
    DoPrintf(_L("FSI_StrucSig:  0x%x\n"),iStructureSig);
    DoPrintf(_L("FSI_FreeCount: 0x%x\n"),iFreeCount);
    DoPrintf(_L("FSI_NxtFree:   0x%x\n"),iNextFree);
    DoPrintf(_L("FSI_TrailSig:  0x%x\n"),iTrainlingSig);
    DoPrintf(_L("========================\n"));
}

TUint32 TFSInfo::FreeClusterCount() const 
{
    return iFreeCount;
}

TUint32 TFSInfo::NextFreeCluster() const
{
    return iNextFree;
}


void TFSInfo::SetFreeClusterCount(TUint32 aFreeCount)
{
    iFreeCount = aFreeCount;
}

void TFSInfo::SetNextFreeCluster(TUint32 aNextCluster)
{
    iNextFree = aNextCluster;
}


//-------------------------------------------------------------------------------------------------------------------

/**
    Deciphers the dos time/date entry information and converts to TTime
*/
TTime Fat_Test_Utils::DosTimeToTTime(TInt aDosTime,TInt aDosDate)
	{
	TInt secMask=0x1F;
	TInt minMask=0x07E0;
	TInt hrMask=0xF800;
	TInt dayMask=0x1F;
	TInt monthMask=0x01E0;
	TInt yearMask=0xFE00;

	TInt secs=(aDosTime&secMask)*2;
	TInt mins=(aDosTime&minMask)>>5;
	TInt hrs=(aDosTime&hrMask)>>11;
	TInt days=(aDosDate&dayMask)-1;
	TMonth months=(TMonth)(((aDosDate&monthMask)>>5)-1);
	TInt years=((aDosDate&yearMask)>>9)+1980;
	
	TDateTime datetime;
	TInt ret=datetime.Set(years,months,days,hrs,mins,secs,0);
	if (ret==KErrNone)
		return(TTime(datetime));
	return(TTime(0));
	}

/**
Converts a TTime to a dos time
*/
TInt Fat_Test_Utils::DosTimeFromTTime(const TTime& aTime)
	{
	TDateTime dateTime=aTime.DateTime();
	TInt dosSecs=dateTime.Second()/2;
	TInt dosMins=dateTime.Minute()<<5;
	TInt dosHrs=dateTime.Hour()<<11;
	return dosSecs|dosMins|dosHrs;
	}

/**
Converts a TTime to a dos date
*/
TInt Fat_Test_Utils::DosDateFromTTime(const TTime& aTime)
	{

	TDateTime dateTime=aTime.DateTime();
	TInt dosDays=dateTime.Day()+1;
	TInt dosMonths=(dateTime.Month()+1)<<5;
	TInt dosYears=(dateTime.Year()-1980)<<9;
	return dosDays|dosMonths|dosYears;
	}


/**
Converts xxx.yyy to standard format aaaaaaaayyy
*/
TBuf8<12> Fat_Test_Utils::DosNameToStdFormat(const TDesC8& aDosName)
	{
    ASSERT(aDosName.Length()>=0 && aDosName.Length()<=KMaxFatFileName);

	TBuf8<12> result;
	Mem::Fill((TUint8*)result.Ptr(),result.MaxSize(),' ');
	TInt dotPos=aDosName.Locate('.');
	if (dotPos==KErrNotFound)
		{
		result=aDosName;
		result.SetLength(11);
		return result;
		}
	result=aDosName.Left(dotPos);
	result.SetLength(11);
	TPtr8 ext(&result[8],3);
	ext=aDosName.Right(aDosName.Length()-dotPos-1);
	return result;
	}

/**
Converts aaaaaaaayyy to dos name format xxx.yyy
*/
TBuf8<12> Fat_Test_Utils::DosNameFromStdFormat(const TDesC8& aStdFormatName)
	{
    ASSERT(aStdFormatName.Length()==11);

	TBuf8<12> result;
	TInt nameLen=aStdFormatName.Locate(' ');
	if (nameLen>8 || nameLen==KErrNotFound)
		nameLen=8;
	result=aStdFormatName.Left(nameLen);
	TPtrC8 ext(&aStdFormatName[8],3);
	TInt extLen=ext.Locate(' ');
	if (extLen)
		result.Append(TChar('.'));
	if (extLen==KErrNotFound)
		extLen=3;
	result.Append(ext.Left(extLen));
    if(result.Length() && result[0]==0x05 )
	    {
	    result[0]=0xE5;
	    }
	return result;
	}

/**
Return the number of VFat entries required to describe a filename of length aNameLength
*/
TInt Fat_Test_Utils::NumberOfVFatEntries(TInt aNameLength)
	{
	TInt numberOfEntries=0;
	if (aNameLength%KMaxVFatEntryName)
		aNameLength++;	//	Include a zero terminator
//	If aNameLength is a exact multiple of KMaxVFatEntryName, don't bother
//	with a zero terminator - it just adds an unnecessary directory entry		
	
	numberOfEntries=(1+(aNameLength/KMaxVFatEntryName));	
	
	if (aNameLength%KMaxVFatEntryName)
		numberOfEntries++;
	
	return(numberOfEntries);
	}

/**
Calculate a checksum to tie the vFat and dos names together
*/
TUint8 Fat_Test_Utils::CalculateShortNameCheckSum(const TDesC8& aShortName)
	{

	TUint8 cksum=0;
	for (TInt i=0;i<11;i++)
		cksum =(TUint8)((((cksum&1)<<7)|((cksum&0xfe)>>1))+aShortName[i]);

	return(cksum);
	}


//-------------------------------------------------------------------------------------------------------------------


#define pDir ((SFatDirEntry*)&iData[0])
const TUint8 KEntryErasedMarker=0xE5;


TFatDirEntry::TFatDirEntry() 
    {
    InitZ();
    }       

/** zero-fill the entry contents  */
void TFatDirEntry::InitZ() 
    {
    Mem::FillZ(iData, KSizeOfFatDirEntry);
    }


/**
@return  ETrue if the Directory entry contains garbage data
*/
TBool TFatDirEntry::IsGarbage() const
    {
    return (iData[0]==0xFF);
    }

/**
Return the Dos name of a directory entry
@return A descriptor containing the Dos name of a directory entry
*/
const TPtrC8 TFatDirEntry::Name() const
	{return TPtrC8((TUint8*)&(pDir->iName),KFatDirNameSize);}

/** @return The attributes for the Directory entry */
TUint TFatDirEntry::Attributes() const
	{return pDir->iAttributes;}

/** @return Time of file creation */
TTime TFatDirEntry::Time() const
	{return DosTimeToTTime(pDir->iTime,pDir->iDate);}

/** @return The Start cluster for the file or directory for this entry  */
TUint32 TFatDirEntry::StartCluster() const		
	{
    const TUint16 KStClustMaskHi = 0x0FFF;	
    return ((pDir->iStartClusterHi&KStClustMaskHi)<<16) | pDir->iStartClusterLo;
    }

/** @return The size of file or directory for this entry  */
TUint32 TFatDirEntry::Size() const
	{return pDir->iSize;}

/** @return True if the entry is erased */
TBool TFatDirEntry::IsErased() const
	{return (TBool)(iData[0]==KEntryErasedMarker);}

/** @return True if the entry refers to the current directory */
TBool TFatDirEntry::IsCurrentDirectory() const
	{return (TBool)(iData[0]==KDotEntryByte && iData[1]==KBlankSpace);}

/** @return True if the Entry refers to the parent directory */
TBool TFatDirEntry::IsParentDirectory() const
	{return (TBool)(iData[0]==KDotEntryByte && iData[1]==KDotEntryByte);}

/** @return True if end of directory */
TBool TFatDirEntry::IsEndOfDirectory() const
	{return (TBool)(iData[0]==0x00);}

/** 
Set the Dos name of a directory entry 
@param aDes A descriptor containg the name
*/
void TFatDirEntry::SetName(const TDesC8& aDes)
	{
    ASSERT(aDes.Length()<=KFatDirNameSize);
	TPtr8 name(pDir->iName,KFatDirNameSize);
	name=aDes;
	}

/** 
Set the file or directory attributes for this entry 
@param anAtts The file or directory attributes
*/
void TFatDirEntry::SetAttributes(TUint anAtts)
	{
	ASSERT(!(anAtts&~KMaxTUint8));
	pDir->iAttributes=(TUint8)anAtts;
	}

/**
Set the creation time and data of the directory entry
@param aTime Creation time of the file or directory
*/
void TFatDirEntry::SetTime(TTime aTime)
	{
	pDir->iTime=(TUint16)DosTimeFromTTime(aTime);
	pDir->iDate=(TUint16)DosDateFromTTime(aTime);
	}

void TFatDirEntry::SetCreateTime(TTime aTime)
	{
	pDir->iTimeC=(TUint16)DosTimeFromTTime(aTime);
	pDir->iDateC=(TUint16)DosDateFromTTime(aTime);
	}

/**
Set the start cluster number of the file or directory refered to by the entry
@param aStartCluster The start cluster number
*/
void TFatDirEntry::SetStartCluster(TUint32 aStartCluster)
	{
	const TUint32 KHalfWordMask	= 0x0000FFFF;
    pDir->iStartClusterLo=(TUint16)(aStartCluster & KHalfWordMask);
	pDir->iStartClusterHi=(TUint16)((aStartCluster>>16) & KHalfWordMask);
	}

/**
Set the size of the file or directory refered to by the entry
@param aFileSize Size of the file
*/
void TFatDirEntry::SetSize(TUint32 aFileSize)
	{pDir->iSize=aFileSize;}

/** Set the directory entry as erased */
void TFatDirEntry::SetErased()
	{iData[0]=KEntryErasedMarker;}

/**  Set the current entry to refer to the current directory */
void TFatDirEntry::SetCurrentDirectory()
	{
	iData[0]='.';
	Mem::Fill(&iData[1],KFatDirNameSize-1,' ');
	}

/** Set the current entry to refer to the parent directory */
void TFatDirEntry::SetParentDirectory()
	{
	iData[0]='.';iData[1]='.';
	Mem::Fill(&iData[2],KFatDirNameSize-2,' ');
	}

/** Set the current entry to be the end of directory marker */
void TFatDirEntry::SetEndOfDirectory()
	{Mem::FillZ(&iData[0],KFatDirNameSize);}

/**  @return True if the entry is the start of a long name set of entries */
TBool TFatDirEntry::IsLongNameStart() const
	{return (TBool)((iData[0]&0x40) != 0);}

/** @return True is the Entry is a VFat entry  */
TBool TFatDirEntry::IsVFatEntry() const
	{return (TBool)(Attributes()==KVFatEntryAttribute && IsEndOfDirectory()==EFalse);}

/** @return The number of following VFat entries */
TInt TFatDirEntry::NumFollowing() const
	{return (iData[0]&0x3F);}


TUint8 TFatDirEntry::CheckSum() const
    {
        ASSERT(IsVFatEntry());
        return iData[13];
    }





















