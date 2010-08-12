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
// various FAT utilities 
// 
//

#include "f32_test_utils.h"
using namespace F32_Test_Utils;

#include "filesystem_fat.h"

//-------------------------------------------------------------------------------------------------------------------

//-- define this macro if it is necessarily to exclude all stuff that uses RFs, consoles etc.
//-- may be useful if this code is used for file server extensions. mmp file is a good place to define this macro.
#ifndef FAT_UTILS_LEAN_AND_MEAN

#include <e32cons.h>
#include <e32math.h>

//-- Note that the writable static data are not allowed in DLLs i.e. plugins or somethig else.
//-- Thus it needs to be thrown away by preprocessing in such a case.

static CConsoleBase* pConsole = NULL;   //-- pointer to the text console for printing out data
static TBool  bPrintOutEnabled = ETrue; //-- global flag, if EFalse, all printing out is disabled



//-------------------------------------------------------------------------------------------------------------------

/**
    Prints out a hex dump of a descriptor contents
    @param  aBuf data descriptor to dump
*/
void  F32_Test_Utils::HexDump(const TDesC8& aBuf)
{
    HexDump(aBuf.Ptr(), aBuf.Size());
}

//-------------------------------------------------------------------------------------------------------------------
/**
    Prints out a hex dump of a buffer
    @param  apBuf   pointer to the data to dump
    @param  aBufLen buffer length
*/
void  F32_Test_Utils::HexDump(const TAny* apBuf, TUint aBufLen)
{
    DoPrintf(_L("~ F32_Test_Utils::HexDump() size:%u\n"), aBufLen);
  
    ASSERT(apBuf);
    
    if(!aBufLen)
        return;

    const TUint colDmpWidth = 16;
    const TUint8* pBuf = (const TUint8*)apBuf;
    TBuf<256> buf1;
    TBuf<64>  buf2;

    TUint dumpPos;
    
    for(dumpPos=0; dumpPos < aBufLen-1; )
    {
        buf1.Format(_L("%06X: "), dumpPos);
        buf2.Zero();

        for(TUint i=0; i<colDmpWidth; ++i)
        {
            buf1.AppendFormat(_L("%02x "), pBuf[dumpPos+i]);
        
            const TChar ch = pBuf[dumpPos+i];
            if(ch.IsPrint())
                buf2.Append(ch);
            else    
                buf2.Append(_L("."));
            

            if(++dumpPos >= aBufLen)
            {    
                while(++i < colDmpWidth)
                {
                    buf1.Append(_L("   "));
                    buf2.Append(_L(" "));
                }

                break;
            }
        }

        buf1.Append(buf2);
        DoPrintf(buf1);
    
    }

    DoPrintf(_L("\n"));

}


//-------------------------------------------------------------------------------------------------------------------
/**
    Compare 2 buffers and print out the difference if there is any.
    Buffer sizes must be the same and non-0

    @param  aBuf1   buffer 1 descriptor
    @param  aBuf2   buffer 2 descriptor
    
    @return ETrue if buffers are the same, EFalse otherwise
*/
TBool F32_Test_Utils::CompareBuffers(const TDesC8& aBuf1, const TDesC8& aBuf2)
{
    return CompareBuffers(aBuf1.Ptr(), aBuf1.Size(), aBuf2.Ptr(), aBuf2.Size());
}

//-------------------------------------------------------------------------------------------------------------------
/**
    Compare 2 buffers and print out the difference if there is any.
    Buffer sizes must be the same and non-0

    @param  apBuf1   pointer to the buffer 1 
    @param  aBuf1Len buffer1 length
    @param  apBuf2   pointer to the buffer 2 
    @param  aBuf2Len buffer2 length

    @return ETrue if buffers are the same, EFalse otherwise
*/
TBool F32_Test_Utils::CompareBuffers(const TAny* apBuf1, TUint aBuf1Len, const TAny* apBuf2, TUint aBuf2Len)
{
    ASSERT(apBuf1 && apBuf2);
    
    if(aBuf1Len != aBuf2Len)
    {
        DoPrintf(_L("~ F32_Test_Utils::CompareBuffers() different sizes! %u:%u\n"), aBuf1Len, aBuf2Len);
        ASSERT(0);
        return EFalse;
    }

    if(!aBuf1Len)
    {//-- empty buffers to compare
        return ETrue;
    }


    const TUint8* pBuf1 = (const TUint8*)apBuf1;
    const TUint8* pBuf2 = (const TUint8*)apBuf2;   
        
    if(!Mem::Compare(pBuf1, aBuf1Len, pBuf2, aBuf2Len))
        return ETrue; //-- buffers are the same. 


    //-- the buffers' contents are different
    TUint diffpos;
    TBuf<256> buf1;
    TBuf<100> buf2;

    const TUint colDmpWidth = 16;
    TBool bBannerPrinted = EFalse;

    //-- dump chunks of the buffer with differences only
    for(diffpos=0; diffpos<aBuf1Len-1; )
    {
        if(pBuf1[diffpos] == pBuf2[diffpos])
            {
            ++diffpos;
            continue;
            }

        if(!bBannerPrinted)
        {
            bBannerPrinted = ETrue;
            DoPrintf(_L("~ F32_Test_Utils::CompareBuffers() buffers' contents are different starting at pos:%u (0x%x). Hexdump:\n"), diffpos, diffpos);

        }

        //-- difference found, dump chunks of the buffer with differences only
        TUint dumpPos = (diffpos >> 4) << 4; //-- round down to 16

        buf1.Format(_L("%06X: "), dumpPos);
        buf2.Format(_L("|"));
        
        for(TUint i=0; i<colDmpWidth; ++i)
        {
            buf1.AppendFormat(_L("%02x"), pBuf1[dumpPos+i]);
            buf2.AppendFormat(_L("%02x"), pBuf2[dumpPos+i]);

            if(i < colDmpWidth-1)
            {
                buf1.Append(_L(" "));
                buf2.Append(_L(" "));
            }

        
            if(++diffpos >= aBuf1Len)
            {//-- pad the dump with spaces
                while(++i < colDmpWidth)
                {
                    buf1.Append(_L("   "));
                    buf2.Append(_L("   "));
                }
            
                break;
            }

        }

        buf1.Append(buf2);
        DoPrintf(buf1);
    }
    
    DoPrintf(_L("\n"));

    return EFalse;
}


//-------------------------------------------------------------------------------------------------------------------
/**
    Set the console where the ouput will go.
    @param  apConsole pointer to the console. if NULL, the print out will be debug port only.
*/
void F32_Test_Utils::SetConsole(CConsoleBase* apConsole)
{
    pConsole = apConsole;
}

/** 
    Enable or disable printing out. See DoPrintf()
    @param  bEnable If ETrue, print out is enabled
    @return previous value of the global bPrintOutEnabled flag
*/
TBool F32_Test_Utils::EnablePrintOutput(TBool bEnable)
{
    TBool bPrevVal = bPrintOutEnabled;
    bPrintOutEnabled = bEnable;

    return bPrevVal;
}

/**
    Print out the drive information.

    @param  aFs             reference to the FS session
    @param  aDrive          drive number
    @return system-wide error codes.
*/
TInt F32_Test_Utils::PrintDrvInfo(RFs &aFs, TInt aDrive)
{
    TInt        nRes;
    TDriveInfo  driveInfo;
    TVolumeInfo volInfo;
    TBuf<256>   Buf;

    //-- get drive info
    nRes = aFs.Drive(driveInfo, aDrive);
    if(nRes != KErrNone)
        return nRes;

    nRes = aFs.Volume(volInfo, aDrive);
    if(nRes != KErrNone)
        return nRes;

    DoPrintf(_L("Drive %c: #%d\n"), 'A'+aDrive, aDrive);

    //-- print the FS name
    nRes = aFs.FileSystemName(Buf, aDrive);
    if(nRes != KErrNone)
        return nRes;
    
    //-- to find out FS sub type
    TFSName fsName;
    nRes = aFs.FileSystemSubType(aDrive, fsName); 
    if(nRes == KErrNone && Buf.CompareF(fsName) !=KErrNone)
    {
        Buf.AppendFormat(_L(" (%S)"), &fsName);
    }
    
        
   DoPrintf(_L("FS name: %S\n"), &Buf);
   
   //-- print drive and media attributes
   DoPrintf(_L("MediaType: 0x%x\n"), driveInfo.iType);
   DoPrintf(_L("DriveAtt:0x%x\n"),driveInfo.iDriveAtt);
   DoPrintf(_L("MediaAtt:0x%x\n"),driveInfo.iMediaAtt);

   //-- volume information
   DoPrintf(_L("VolId:0x%x\n"),volInfo.iUniqueID);
   DoPrintf(_L("VolSz:%ld (%ldK)\n"),volInfo.iSize, volInfo.iSize/1024);
   DoPrintf(_L("Free:%ld (%ldK)\n"),volInfo.iFree, volInfo.iFree/1024);

   return KErrNone;
}

//-------------------------------------------------------------------------------------------------------------------


/**
    Fill a media region with a given byte pattern
    
    @param  aFs             reference to the FS session
    @param  aDrive          drive number
    @param  aMediaStartPos  media region start position  
    @param  aMediaEndPos    media region end position
    @param  aBytePattern    byte to fill the media region with
    
    @return system-wide error codes.
*/
TInt F32_Test_Utils::FillMedia(RFs &aFs, TInt aDrive, TInt64 aMediaStartPos, TInt64 aMediaEndPos, TUint8 aBytePattern/*=0*/)
{
    DoPrintf(_L("~ F32_Test_Utils::FillMedia() drv:%d, from:%u to:%u\n"),aDrive, (TUint32)aMediaStartPos, (TUint32)aMediaEndPos);
   
    ASSERT(aMediaStartPos<=aMediaEndPos && aMediaStartPos >=0  && aMediaEndPos >=0);

    TInt  nRes = KErrNone;
    RBuf8 buf;
    const TUint32 KBufSz=65536; //-- zero-buffer size, bytes
    
    //-- create a buffer, filled with given pattern
    nRes = buf.CreateMax(KBufSz);
    ASSERT(nRes == KErrNone);
    buf.Fill(aBytePattern);

    TUint32 rem = (TUint32)(aMediaEndPos - aMediaStartPos);
    while(rem)
    {
        const TUint32 bytesToWrite=Min(rem, KBufSz);
        TPtrC8 ptrData(buf.Ptr(), bytesToWrite);

        nRes = MediaRawWrite(aFs, aDrive, aMediaStartPos, ptrData); 
        if(nRes != KErrNone && nRes != KErrDiskFull)
            break;

        aMediaStartPos+=bytesToWrite;
        rem-=bytesToWrite;
    }

    buf.Close();

    return nRes;    
}


//-------------------------------------------------------------------------------------------------------------------

/**
    Raw read from the media.

    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aMediaPos   media position 
    @param  aLen        how many bytes to read
    @param  aData       descriptor for the data

    @return system-wide error code.
*/
TInt F32_Test_Utils::MediaRawRead(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TUint32 aLen, TDes8& aData)
{
    TInt nRes=KErrNone;
    TRAP(nRes, DoMediaRawReadL(aFs, aDrive, aMediaPos, aLen, aData));
    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Raw write to the media.

    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aMediaPos   media position 
    @param  aData       descriptor with the data to write

    @return system-wide error code.
*/
TInt F32_Test_Utils::MediaRawWrite(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TDesC8& aData)
{
    TInt nRes=KErrNone;
    TRAP(nRes, DoMediaRawWriteL(aFs, aDrive, aMediaPos, aData));
    return nRes;
}


//-------------------------------------------------------------------------------------------------------------------
void F32_Test_Utils::DoMediaRawReadL(RFs &aFs, TInt aDrive, TInt64 aMediaPos, TUint32 aLen, TDes8& aData)
{
    ASSERT(aMediaPos>=0);
    ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);

    if((TUint32)aData.MaxSize() < aLen)
        User::Leave(KErrArgument);

    if(aLen == 0)
        return;

    aData.SetLength(aLen);

    RRawDisk  rawDisk;
    CleanupClosePushL(rawDisk);
    
    User::LeaveIfError(rawDisk.Open(aFs, aDrive));
    User::LeaveIfError(rawDisk.Read(aMediaPos, aData));

    CleanupStack::PopAndDestroy(&rawDisk);
}

//-------------------------------------------------------------------------------------------------------------------
void F32_Test_Utils::DoMediaRawWriteL(RFs &aFs, TInt aDrive, TInt64 aMediaPos, const TDesC8& aData)
{
    ASSERT(aMediaPos>=0);
    ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
  
    if(aData.Size() == 0)
        return;

    RRawDisk  rawDisk;
    CleanupClosePushL(rawDisk);
    
    User::LeaveIfError(rawDisk.Open(aFs, aDrive));
    User::LeaveIfError(rawDisk.Write(aMediaPos, (TDesC8&)aData));

    CleanupStack::PopAndDestroy(&rawDisk);
}

//-------------------------------------------------------------------------------------------------------------------

_LIT(KFsName_LFFS,  "lffs");
_LIT(KFsName_Win32, "Win32");
_LIT(KFsName_ExFAT, "ExFat");
_LIT(KFsName_AutoMonuter, "automounter");
_LIT(KFsName_HVFS, "HVFS");

/**  @return ETrue if "Automounter" FS is mounted on this drive */
TBool F32_Test_Utils::Is_Automounter(RFs &aFs, TInt aDrive)
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName f;
	TInt r = aFs.FileSystemName(f, aDrive);
    __ASSERT_ALWAYS((r==KErrNone) && (f.Length()>0), User::Invariant());

    return (f.CompareF(KFsName_AutoMonuter) == 0 );

}

/** @return ETrue if "lffs" FS is mounted on this drive */
TBool F32_Test_Utils::Is_Lffs(RFs &aFs, TInt aDrive)
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName f;
	TInt r = aFs.FileSystemName(f, aDrive);
    __ASSERT_ALWAYS((r==KErrNone) && (f.Length()>0), User::Invariant());

    return (f.CompareF(KFsName_LFFS) == 0 );

}
   
/** @return ETrue if "Win32" FS is mounted on this drive (i.e this is emulator's drive C:) */
TBool F32_Test_Utils::Is_Win32(RFs &aFs, TInt aDrive)   
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName f;
	TInt r = aFs.FileSystemName(f, aDrive);
    __ASSERT_ALWAYS((r==KErrNone) && (f.Length()>0), User::Invariant());

    return (f.CompareF(KFsName_Win32) == 0 );
}

/** @return ETrue if "HVFS" is mounted on this drive (i.e PlatSim's drive C:) */
TBool F32_Test_Utils::Is_HVFS(RFs &aFs, TInt aDrive)
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName f;
	TInt r = aFs.FileSystemName(f, aDrive);
    __ASSERT_ALWAYS((r==KErrNone) && (f.Length()>0), User::Invariant());

    return (f.CompareF(KFsName_HVFS) == 0);
}

/** @return ETrue if "HVFS" or "Win32" FS is mounted on this drive
 * 			(i.e drive C: of PlatSim or the emulator) */
TBool F32_Test_Utils::Is_SimulatedSystemDrive(RFs &aFs, TInt aDrive)
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName f;
	TInt r = aFs.FileSystemName(f, aDrive);
    __ASSERT_ALWAYS((r==KErrNone) && (f.Length()>0), User::Invariant());

    return (f.CompareF(KFsName_HVFS) == 0 || f.CompareF(KFsName_Win32) == 0);
}

/** @return ETrue if the filesystem if FAT (fat12/16/32) */
TBool F32_Test_Utils::Is_Fat(RFs &aFs, TInt aDrive)
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName fsName;
	TInt nRes = aFs.FileSystemName(fsName, aDrive);
    __ASSERT_ALWAYS((nRes==KErrNone) && (fsName.Length()>0), User::Invariant());

    if(fsName.CompareF(KFileSystemName_FAT) == 0)
        return ETrue; //-- "FAT" FS is explicitly mounted on this drive

    //-- try analyse file system subtype for the case of automounter FS
    nRes = aFs.FileSystemSubType(aDrive,fsName); 
    if(nRes !=KErrNone)
        return EFalse;

    return (fsName.CompareF(KFSSubType_FAT16) == 0 || fsName.CompareF(KFSSubType_FAT32) == 0 || fsName.CompareF(KFSSubType_FAT12) == 0);
}

/** returns ETrue if "exFAT" FS is mounted on this drive */
TBool F32_Test_Utils::Is_ExFat(RFs &aFs, TInt aDrive)
{
	ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    TFSName fsName;
	TInt nRes = aFs.FileSystemName(fsName, aDrive);
    __ASSERT_ALWAYS((nRes==KErrNone) && (fsName.Length()>0), User::Invariant());

    if(fsName.CompareF(KFsName_ExFAT) == 0 )
        return ETrue; //-- "exFAT" FS is explicitly mounted on this drive

    //-- try analyse file system subtype for the case of automounter FS
    nRes = aFs.FileSystemSubType(aDrive,fsName); 
    if(nRes !=KErrNone)
        return EFalse;

    return (fsName.CompareF(KFsName_ExFAT) == 0);
}

/** @return ETrue if the filesystem if FAT32 */
TBool F32_Test_Utils::Is_Fat32(RFs &aFs, TInt aDrive)
{
    if(!Is_Fat(aFs, aDrive))
        return EFalse;

    TFSName fsName;
    TInt nRes = aFs.FileSystemSubType(aDrive,fsName); 
    
    if(nRes !=KErrNone)
        return EFalse;

    return (fsName.CompareF(KFSSubType_FAT32) == 0);

}

/** @return ETrue if the filesystem if FAT16 */
TBool F32_Test_Utils::Is_Fat16(RFs &aFs, TInt aDrive)
{
    if(!Is_Fat(aFs, aDrive))
        return EFalse;

    TFSName fsName;
    TInt nRes = aFs.FileSystemSubType(aDrive,fsName); 
    
    if(nRes !=KErrNone)
        return EFalse;

    return (fsName.CompareF(KFSSubType_FAT16) == 0);

}

/** @return ETrue if the filesystem if FAT12 */
TBool F32_Test_Utils::Is_Fat12(RFs &aFs, TInt aDrive)
{
    if(!Is_Fat(aFs, aDrive))
        return EFalse;

    TFSName fsName;
    TInt nRes = aFs.FileSystemSubType(aDrive,fsName); 
    
    if(nRes !=KErrNone)
        return EFalse;

    return (fsName.CompareF(KFSSubType_FAT12) == 0);

}


static void DoCreateCheckableFileL(RFile64& aFile, TUint64 aSize)
{
    
    ASSERT(aSize >= TMD5::HashSize);

    //-- 1. set file size
    User::LeaveIfError(aFile.SetSize((TInt)aSize));

    //-- 2. leave place for the 16-bytes MD5 hash in the file beginning
    TInt64 filePos = TMD5::HashSize;
    User::LeaveIfError(aFile.Seek(ESeekStart,filePos));
    aSize-=TMD5::HashSize;
    
    TMD5 md5Hash;
    
    //-- 3. fill the file with random bytes
    const TInt KBufSize=65536; //-- buffer size for writing data
    TInt64 rndSeed = aSize % 43283;
    if(!rndSeed)
        rndSeed = 33521; 

    RBuf8 buf;
    buf.CreateMaxL(KBufSize);
    
    TUint64 prevSz = aSize;
    while(aSize)
    {
        //-- initialize buffer with random rubbish
        for(TInt i=0; i<KBufSize; ++i)
        {
            buf[i] = (TUint8)Math::Rand(rndSeed);
        }

        const TUint32 nBytes = (TUint32)Min((TUint64)KBufSize, aSize);
        TPtrC8 ptrData(buf.Ptr(), nBytes);
        User::LeaveIfError(aFile.Write(ptrData)); //-- write data to the file
        md5Hash.Update(ptrData); //-- update MD5 hash        
       
        aSize-=nBytes;
        
        if((prevSz - aSize) >= K1MegaByte)
        {
            prevSz = aSize;
            DoPrintf(_L("."));
        }
    }


    buf.Close();
    DoPrintf(_L("\n"));

    //-- 4. write MD5 hash to the beginning of the file
    filePos = 0;
    User::LeaveIfError(aFile.Seek(ESeekStart,filePos));
    User::LeaveIfError(aFile.Write(md5Hash.Final()));

}


static void DoVerifyCheckableFileL(RFile64& aFile)
{
    TInt64 fileSize;
    User::LeaveIfError(aFile.Size(fileSize));

    if(fileSize < TMD5::HashSize)
       User::Leave(KErrCorrupt); //-- MD5 hash is 16 bytes, it's the minimal file size

    //-- 1. read MD5 header from the file
    TBuf8<TMD5::HashSize> md5Header(TMD5::HashSize);
    User::LeaveIfError(aFile.Read(md5Header));
    fileSize -= TMD5::HashSize;

    //-- 2. read the rest of the data and calculate the checksum
    TMD5 md5Hash;
    RBuf8 buf;
    
    const TInt KBufSize=65536; //-- buffer size for writing data
    buf.CreateMaxL(KBufSize);
    
    TUint64 prevSz = fileSize;
    while(fileSize)
    {
        User::LeaveIfError(aFile.Read(buf)); //-- read data from the file
		if (buf.Length() == 0)
			User::Leave(KErrEof);
        md5Hash.Update(buf); //-- update MD5 hash        
        
        fileSize-=buf.Length();

        if((prevSz - fileSize) >= K1MegaByte)
        {
            prevSz = fileSize;
            DoPrintf(_L("."));
        }
    }

    buf.Close();
    DoPrintf(_L("\n"));

    if(md5Hash.Final() != md5Header)
        User::Leave(KErrCorrupt); //-- the file is corrupt
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Creates a file filled with random data. The file has a 16-byte MD5 header at the start, so it is possible to
    verify the data validity later. Thus the minimal file size is 16 bytes. The file with the iven name may already
    exist; in this case it will be replaced.

    @param  aFs         reference to the FS session
    @param  aFileName   name of the file to be created / replaced
    @param  aSize       size of the file; 16 bytes min.

    @return Standard error code
*/
TInt  F32_Test_Utils::CreateCheckableStuffedFile(RFs& aFs, const TDesC& aFileName, TUint64 aSize)
{

    DoPrintf(_L("~ F32_Test_Utils::CreateCheckableStuffedFile() file:%S, Size:%LU\n"), &aFileName, aSize);

    if(aSize < TMD5::HashSize)
        return KErrArgument; //-- MD5 hash is 16 bytes, it's the minimal file size

    RFile64 file;

    //-- 1. create a file
    TInt nRes = file.Replace(aFs, aFileName, EFileWrite);
    if(nRes != KErrNone)
        return nRes;

    TRAP(nRes, DoCreateCheckableFileL(file, aSize));

    file.Close();
    return nRes;
}


/**
    Verify previously created file that has MD5 header at the beginning. See  CreateCheckableStuffedFile(...)

    @param  aFs         reference to the FS session
    @param  aFileName   name of the file to be verified

    @return Standard error code. KErrNone if the file contents matches the MD5 header.
*/
TInt  F32_Test_Utils::VerifyCheckableFile(RFs& aFs, const TDesC& aFileName)
{
    DoPrintf(_L("~ F32_Test_Utils::VerifyCheckableFile() file:%S\n"), &aFileName);

    RFile64 file;

    //-- 1. create a file
    TInt nRes = file.Open(aFs, aFileName, EFileRead);
    if(nRes != KErrNone)
        return nRes;

    TRAP(nRes, DoVerifyCheckableFileL(file));

    file.Close();
    return nRes;

}


/**
    Create an empty file (not filled with anything). The size of the file is set by using RFile::SetSize().
    For FAT this will result in allocating a cluster chain in FAT fable, for the file systems that support sparse files (like LFFS)
    this might not work as expected.

    @param  aFs         reference to the FS session
    @param  aFileName   name of the file to be created / replaced
    @param  aSize       size of the file

    @return Standard error code
*/
TInt  F32_Test_Utils::CreateEmptyFile(RFs& aFs, const TDesC& aFileName, TUint64 aSize)
{
    DoPrintf(_L("~ F32_Test_Utils::CreateEmptyFile() file:%S, sz:%LU\n"), &aFileName, aSize);

    RFile64 file;

    //-- 1. create a file
    TInt nRes = file.Replace(aFs, aFileName, EFileWrite);
    if(nRes != KErrNone)
        return nRes;

    //-- 2. set file size
    nRes = file.SetSize(aSize);

    file.Close();

    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

/**
    Dismount and mount the filesystem again, optionally taking time when the mount starts.
    The FS can have extensions added into it; this function will handle only the primary extension (if it is present) and
    will mont the FS with it. Non-primary extensions are not supported yet.


    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  apTimeMountStart pointer to the TTime object, that can be called TTime::UniversalTime() on mount start (this can be
                             used for measuring time taken to mount the FS).
                             if NULL, no action will be taken.

    @return error code from the RFs::MountFileSystem()

*/
TInt  F32_Test_Utils::RemountFS(RFs& aFs, TInt aDrive, TTime* apTimeMountStart/*=NULL*/)
{
    TInt nRes;
    DoPrintf(_L("~ F32_Test_Utils::RemountingFS at drive:%d\n"), aDrive);    

    TFSDescriptor fsDescriptor;

    //-- 1. get current FS information 
    nRes = GetFileSystemDescriptor(aFs, aDrive, fsDescriptor);
    if(nRes != KErrNone)
        return nRes;

    //-- 2. dismount the file system
    if(fsDescriptor.iPExtName.Length() > 0)
    {
        DoPrintf(_L("~ Dismounting FS:%S with ext:%S\n"), &fsDescriptor.iFsName, &fsDescriptor.iPExtName);
    }
    else
    {
        DoPrintf(_L("~ Dismounting FS:%S\n"), &fsDescriptor.iFsName);    
    }

    nRes = aFs.DismountFileSystem(fsDescriptor.iFsName, aDrive);
    if(nRes != KErrNone)
    {
        ASSERT(0);
        return nRes;
    }

    //-- 3. mount it again
    if(apTimeMountStart)
        apTimeMountStart->UniversalTime(); //-- take Mount start time

    
    nRes = MountFileSystem(aFs, aDrive, fsDescriptor);
    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------

TFSDescriptor::TFSDescriptor()
{
    Init();
}

void TFSDescriptor::Init()
{
    iFsName.SetLength(0);
    iPExtName.SetLength(0);
    iDriveSynch = EFalse;
}

TBool TFSDescriptor::operator==(const TFSDescriptor& aRhs) const
{
    ASSERT(this != &aRhs);
    return (iFsName.CompareF(aRhs.iFsName) == 0 && iPExtName.CompareF(aRhs.iPExtName) == 0 && iDriveSynch == aRhs.iDriveSynch);
}


//-------------------------------------------------------------------------------------------------------------------
/**
    Gets the information about file system mounted on a drive. This information can be later used for mounting this FS back if it is going to be dismounted

    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aFsDesc     file system descriptor
    
    @return standard error code
*/
TInt F32_Test_Utils::GetFileSystemDescriptor(RFs &aFs, TInt aDrive, TFSDescriptor& aFsDesc)
{
    TInt nRes;

    //-- 1. get file system name
    nRes = aFs.FileSystemName(aFsDesc.iFsName, aDrive);
    if(nRes != KErrNone)
    {
        ASSERT(0);
        return nRes;
    }
    
    //-- 2. find out if the drive sync/async
    TPckgBuf<TBool> drvSyncBuf;
    nRes = aFs.QueryVolumeInfoExt(aDrive, EIsDriveSync, drvSyncBuf);
    if(nRes != KErrNone)
    {//-- pretend that the drive is asynch. in the case of file system being corrupted. this is 99.9% true
       aFsDesc.iDriveSynch = EFalse;
    }
    else
    {
        aFsDesc.iDriveSynch = drvSyncBuf();
    }

    //-- 3. find out primary extension name if it is present; we will need to add it again when mounting the FS
    //-- other extensions (non-primary) are not supported yet
    nRes = aFs.ExtensionName(aFsDesc.iPExtName, aDrive, 0);
    if(nRes != KErrNone)
    {
        aFsDesc.iPExtName.SetLength(0);
    }

    //-- 3.1 check if the drive has non-primary extensions, fail in this case
    TBuf<40> extName;
    nRes = aFs.ExtensionName(extName, aDrive, 1);
    if(nRes == KErrNone)
    {   
        DoPrintf(_L("~ F32_Test_Utils::GetFileSystemDescriptor: Non-primary extensions are not supported!\n"));
        return KErrNotSupported;
    }


    return KErrNone;
}

//-------------------------------------------------------------------------------------------------------------------
/**
    Mount the file system by the information provided in the FS descriptor

    @param  aFs         reference to the FS session
    @param  aDrive      drive number
    @param  aFsDesc     file system descriptor containing all necessary information to mount the FS.
    
    @return standard error code
*/
TInt F32_Test_Utils::MountFileSystem(RFs &aFs, TInt aDrive, const TFSDescriptor& aFsDesc)
{
    DoPrintf(_L("~ F32_Test_Utils::MountFileSystem() drive:%d Name:%S\n"), aDrive, &aFsDesc.iFsName);  
    
    TInt nRes;
    if(aFsDesc.iFsName.Length() <=0 )
    {
        ASSERT(0);
        return KErrArgument;
    }  


    //-- mount File system 
    const TBool bPrimaryExt = (aFsDesc.iPExtName.Length() > 0);

    if(bPrimaryExt)
    {//-- we need to mount FS with the primary extension
        nRes = aFs.AddExtension(aFsDesc.iPExtName);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
        {
            ASSERT(0);
            return nRes;
        }
        
        nRes = aFs.MountFileSystem(aFsDesc.iFsName, aFsDesc.iPExtName, aDrive, aFsDesc.iDriveSynch);
    }
    else
    {//-- the FS did not have primary extension
        nRes = aFs.MountFileSystem(aFsDesc.iFsName, aDrive, aFsDesc.iDriveSynch);
    }


    return nRes;
}

//-------------------------------------------------------------------------------------------------------------------
/**
    Format volume, regardless the file system installed.
    
    @param  aFs             reference to the FS session
    @param  aDrive          drive number
    @param  aQuickFormat    if True, a quick format will be performed. otherwise - full
    @return system-wide error codes.
*/
TInt F32_Test_Utils::FormatDrive(RFs &aFs, TInt aDrive, TBool aQuickFormat)
{
    TPtrC fmtTypeName = (aQuickFormat ? _L("Quick") : _L("Full"));
    DoPrintf(_L("~ F32_Test_Utils::FormatDrive() drv:%d, type:%S\n"),aDrive, &fmtTypeName);
   
    ASSERT(aDrive >= EDriveA && aDrive <= EDriveZ);
    
    RFormat format;
    TUint   fmtMode=0;
    TInt    fmtCnt=0;
    TInt    prevCnt;
    TInt    nRes;

    if(aQuickFormat) 
        fmtMode |= EQuickFormat;

    //if(aForceErase)
    //    fmtMode |= EForceErase;

    TBuf<10> drvName;
    drvName.Format(_L("%C:"),'A'+aDrive);
    
    nRes = format.Open(aFs, drvName, fmtMode, fmtCnt);
    if(nRes!=KErrNone)
        goto Fail;

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
    DoPrintf(_L("~ F32_Test_Utils::FormatFatDrive() failed! code:%d\n"), nRes);

    return nRes;
}


#endif //FAT_UTILS_LEAN_AND_MEAN


//-------------------------------------------------------------------------------------------------------------------
/**
    printing interface. Prints out to the console (if is set) and to the debug interface
    if pConsole is NULL will print to the debug port only.
*/ 
void F32_Test_Utils::DoPrintf(TRefByValue<const TDesC> aFmt,...)
{
#ifndef FAT_UTILS_LEAN_AND_MEAN
    if(!bPrintOutEnabled) 
        return; //-- disabled by global flag
#endif //FAT_UTILS_LEAN_AND_MEAN

    VA_LIST list;
    VA_START(list, aFmt);
    
    TBuf<0x100> buf;
    buf.FormatList(aFmt, list); //-- ignore overflows

#ifndef FAT_UTILS_LEAN_AND_MEAN
    if(pConsole)
    {
        pConsole->Write(buf);
    }
#endif //FAT_UTILS_LEAN_AND_MEAN
    
    const TInt bufLen = buf.Length();
    if(bufLen >0 && buf[bufLen-1] == '\n')
    {
        buf.Insert(bufLen-1, _L("\r"));
    }

    RDebug::RawPrint(buf);
}

//-------------------------------------------------------------------------------------------------------------------

TBool F32_Test_Utils::IsPowerOf2(TUint32 aVal)
    {
    if (aVal==0)
        return EFalse;

    return !(aVal & (aVal-1));
    }


//-------------------------------------------------------------------------------------------------------------------
TUint32 F32_Test_Utils::Log2(TUint32 aVal)
{
    __ASSERT_COMPILE(sizeof(TUint32) == 4);
    ASSERT(aVal);

    TUint32 bitPos=31;

    if(!(aVal >> 16)) {bitPos-=16; aVal<<=16;}
    if(!(aVal >> 24)) {bitPos-=8;  aVal<<=8 ;}
    if(!(aVal >> 28)) {bitPos-=4;  aVal<<=4 ;}
    if(!(aVal >> 30)) {bitPos-=2;  aVal<<=2 ;}
    if(!(aVal >> 31)) {bitPos-=1;}
    
    return bitPos;
}


//-------------------------------------------------------------------------------------------------------------------

//###################################################################################################################
//#     TMD5 class implementation 
//###################################################################################################################


#define T_MASK ((TUint32)~0)
#define T1 /* 0xd76aa478 */ (T_MASK ^ 0x28955b87)
#define T2 /* 0xe8c7b756 */ (T_MASK ^ 0x173848a9)
#define T3    0x242070db
#define T4 /* 0xc1bdceee */ (T_MASK ^ 0x3e423111)
#define T5 /* 0xf57c0faf */ (T_MASK ^ 0x0a83f050)
#define T6    0x4787c62a
#define T7 /* 0xa8304613 */ (T_MASK ^ 0x57cfb9ec)
#define T8 /* 0xfd469501 */ (T_MASK ^ 0x02b96afe)
#define T9    0x698098d8
#define T10 /* 0x8b44f7af */ (T_MASK ^ 0x74bb0850)
#define T11 /* 0xffff5bb1 */ (T_MASK ^ 0x0000a44e)
#define T12 /* 0x895cd7be */ (T_MASK ^ 0x76a32841)
#define T13    0x6b901122
#define T14 /* 0xfd987193 */ (T_MASK ^ 0x02678e6c)
#define T15 /* 0xa679438e */ (T_MASK ^ 0x5986bc71)
#define T16    0x49b40821
#define T17 /* 0xf61e2562 */ (T_MASK ^ 0x09e1da9d)
#define T18 /* 0xc040b340 */ (T_MASK ^ 0x3fbf4cbf)
#define T19    0x265e5a51
#define T20 /* 0xe9b6c7aa */ (T_MASK ^ 0x16493855)
#define T21 /* 0xd62f105d */ (T_MASK ^ 0x29d0efa2)
#define T22    0x02441453
#define T23 /* 0xd8a1e681 */ (T_MASK ^ 0x275e197e)
#define T24 /* 0xe7d3fbc8 */ (T_MASK ^ 0x182c0437)
#define T25    0x21e1cde6
#define T26 /* 0xc33707d6 */ (T_MASK ^ 0x3cc8f829)
#define T27 /* 0xf4d50d87 */ (T_MASK ^ 0x0b2af278)
#define T28    0x455a14ed
#define T29 /* 0xa9e3e905 */ (T_MASK ^ 0x561c16fa)
#define T30 /* 0xfcefa3f8 */ (T_MASK ^ 0x03105c07)
#define T31    0x676f02d9
#define T32 /* 0x8d2a4c8a */ (T_MASK ^ 0x72d5b375)
#define T33 /* 0xfffa3942 */ (T_MASK ^ 0x0005c6bd)
#define T34 /* 0x8771f681 */ (T_MASK ^ 0x788e097e)
#define T35    0x6d9d6122
#define T36 /* 0xfde5380c */ (T_MASK ^ 0x021ac7f3)
#define T37 /* 0xa4beea44 */ (T_MASK ^ 0x5b4115bb)
#define T38    0x4bdecfa9
#define T39 /* 0xf6bb4b60 */ (T_MASK ^ 0x0944b49f)
#define T40 /* 0xbebfbc70 */ (T_MASK ^ 0x4140438f)
#define T41    0x289b7ec6
#define T42 /* 0xeaa127fa */ (T_MASK ^ 0x155ed805)
#define T43 /* 0xd4ef3085 */ (T_MASK ^ 0x2b10cf7a)
#define T44    0x04881d05
#define T45 /* 0xd9d4d039 */ (T_MASK ^ 0x262b2fc6)
#define T46 /* 0xe6db99e5 */ (T_MASK ^ 0x1924661a)
#define T47    0x1fa27cf8
#define T48 /* 0xc4ac5665 */ (T_MASK ^ 0x3b53a99a)
#define T49 /* 0xf4292244 */ (T_MASK ^ 0x0bd6ddbb)
#define T50    0x432aff97
#define T51 /* 0xab9423a7 */ (T_MASK ^ 0x546bdc58)
#define T52 /* 0xfc93a039 */ (T_MASK ^ 0x036c5fc6)
#define T53    0x655b59c3
#define T54 /* 0x8f0ccc92 */ (T_MASK ^ 0x70f3336d)
#define T55 /* 0xffeff47d */ (T_MASK ^ 0x00100b82)
#define T56 /* 0x85845dd1 */ (T_MASK ^ 0x7a7ba22e)
#define T57    0x6fa87e4f
#define T58 /* 0xfe2ce6e0 */ (T_MASK ^ 0x01d3191f)
#define T59 /* 0xa3014314 */ (T_MASK ^ 0x5cfebceb)
#define T60    0x4e0811a1
#define T61 /* 0xf7537e82 */ (T_MASK ^ 0x08ac817d)
#define T62 /* 0xbd3af235 */ (T_MASK ^ 0x42c50dca)
#define T63    0x2ad7d2bb
#define T64 /* 0xeb86d391 */ (T_MASK ^ 0x14792c6e)


TMD5::TMD5()
{
    Reset();
}

//-------------------------------------------------------------------------------------------------------------------

void TMD5::Md5_process(const TUint8 *data /*[64]*/)
{
    TUint32
    a = iState.abcd[0], b = iState.abcd[1],
    c = iState.abcd[2], d = iState.abcd[3];
    TUint32 t;
    TUint32 xbuf[16];
    const TUint32 *X;

    {
        static const TInt w = 1;
        if (*((const TUint8 *)&w)) 
        {
            if (!((data - (const TUint8 *)0) & 3)) {
            X = (const TUint32 *)data;
            } else {
            memcpy(xbuf, data, 64);
            X = xbuf;
            }
        }
        else
        {
            const TUint8 *xp = data;
            TInt i;

            X = xbuf;       /* (dynamic only) */
            for (i = 0; i < 16; ++i, xp += 4)
            xbuf[i] = xp[0] + (xp[1] << 8) + (xp[2] << 16) + (xp[3] << 24);
        }
    }

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + F(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  0,  7,  T1);
    SET(d, a, b, c,  1, 12,  T2);
    SET(c, d, a, b,  2, 17,  T3);
    SET(b, c, d, a,  3, 22,  T4);
    SET(a, b, c, d,  4,  7,  T5);
    SET(d, a, b, c,  5, 12,  T6);
    SET(c, d, a, b,  6, 17,  T7);
    SET(b, c, d, a,  7, 22,  T8);
    SET(a, b, c, d,  8,  7,  T9);
    SET(d, a, b, c,  9, 12, T10);
    SET(c, d, a, b, 10, 17, T11);
    SET(b, c, d, a, 11, 22, T12);
    SET(a, b, c, d, 12,  7, T13);
    SET(d, a, b, c, 13, 12, T14);
    SET(c, d, a, b, 14, 17, T15);
    SET(b, c, d, a, 15, 22, T16);
#undef SET

#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define SET(a, b, c, d, k, s, Ti)\
    t = a + G(b,c,d) + X[k] + Ti;\
    a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  1,  5, T17);
    SET(d, a, b, c,  6,  9, T18);
    SET(c, d, a, b, 11, 14, T19);
    SET(b, c, d, a,  0, 20, T20);
    SET(a, b, c, d,  5,  5, T21);
    SET(d, a, b, c, 10,  9, T22);
    SET(c, d, a, b, 15, 14, T23);
    SET(b, c, d, a,  4, 20, T24);
    SET(a, b, c, d,  9,  5, T25);
    SET(d, a, b, c, 14,  9, T26);
    SET(c, d, a, b,  3, 14, T27);
    SET(b, c, d, a,  8, 20, T28);
    SET(a, b, c, d, 13,  5, T29);
    SET(d, a, b, c,  2,  9, T30);
    SET(c, d, a, b,  7, 14, T31);
    SET(b, c, d, a, 12, 20, T32);
#undef SET

#define H(x, y, z) ((x) ^ (y) ^ (z))
#define SET(a, b, c, d, k, s, Ti)\
    t = a + H(b,c,d) + X[k] + Ti;\
    a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  5,  4, T33);
    SET(d, a, b, c,  8, 11, T34);
    SET(c, d, a, b, 11, 16, T35);
    SET(b, c, d, a, 14, 23, T36);
    SET(a, b, c, d,  1,  4, T37);
    SET(d, a, b, c,  4, 11, T38);
    SET(c, d, a, b,  7, 16, T39);
    SET(b, c, d, a, 10, 23, T40);
    SET(a, b, c, d, 13,  4, T41);
    SET(d, a, b, c,  0, 11, T42);
    SET(c, d, a, b,  3, 16, T43);
    SET(b, c, d, a,  6, 23, T44);
    SET(a, b, c, d,  9,  4, T45);
    SET(d, a, b, c, 12, 11, T46);
    SET(c, d, a, b, 15, 16, T47);
    SET(b, c, d, a,  2, 23, T48);
#undef SET

#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define SET(a, b, c, d, k, s, Ti)\
    t = a + I(b,c,d) + X[k] + Ti;\
    a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  0,  6, T49);
    SET(d, a, b, c,  7, 10, T50);
    SET(c, d, a, b, 14, 15, T51);
    SET(b, c, d, a,  5, 21, T52);
    SET(a, b, c, d, 12,  6, T53);
    SET(d, a, b, c,  3, 10, T54);
    SET(c, d, a, b, 10, 15, T55);
    SET(b, c, d, a,  1, 21, T56);
    SET(a, b, c, d,  8,  6, T57);
    SET(d, a, b, c, 15, 10, T58);
    SET(c, d, a, b,  6, 15, T59);
    SET(b, c, d, a, 13, 21, T60);
    SET(a, b, c, d,  4,  6, T61);
    SET(d, a, b, c, 11, 10, T62);
    SET(c, d, a, b,  2, 15, T63);
    SET(b, c, d, a,  9, 21, T64);
#undef SET

    iState.abcd[0] += a;
    iState.abcd[1] += b;
    iState.abcd[2] += c;
    iState.abcd[3] += d;

}

//-------------------------------------------------------------------------------------------------------------------
void TMD5::Md5_append(const TUint8 *data, TInt nbytes)
{
    const TUint8 *p = data;
    
    TInt left = nbytes;

    TInt offset = (iState.count[0] >> 3) & 63;
    TUint32 nbits = (TUint32)(nbytes << 3);

    if (nbytes <= 0)
    return;

    iState.count[1] += nbytes >> 29;
    iState.count[0] += nbits;
    if (iState.count[0] < nbits)
    iState.count[1]++;

    if (offset) 
    {
    TInt copy = (offset + nbytes > 64 ? 64 - offset : nbytes);

    memcpy(iState.buf + offset, p, copy);
    if (offset + copy < 64)
        return;
    p += copy;
    left -= copy;
    Md5_process(iState.buf);
    }

    for (; left >= 64; p += 64, left -= 64)
        Md5_process(p);

    if (left)
        memcpy(iState.buf, p, left);

}


void TMD5::Md5_finish()
{
    static const TUint8 pad[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    TUint8 data[8];
    TInt i;

    for (i = 0; i < 8; ++i)
    data[i] = (TUint8)(iState.count[i >> 2] >> ((i & 3) << 3));
    Md5_append(pad, ((55 - (iState.count[0] >> 3)) & 63) + 1);
    Md5_append(data, 8);
    for (i = 0; i < 16; ++i)
        iDigest[i] = (TUint8)(iState.abcd[i >> 2] >> ((i & 3) << 3));

}

//-------------------------------------------------------------------------------------------------------------------

/** reset MD5 to initial state */
void TMD5::Reset()
{
    iState.count[0] = iState.count[1] = 0;
    iState.abcd[0] = 0x67452301;
    iState.abcd[1] = /*0xefcdab89*/ T_MASK ^ 0x10325476;
    iState.abcd[2] = /*0x98badcfe*/ T_MASK ^ 0x67452301;
    iState.abcd[3] = 0x10325476;
}


/**
    Update MD5 with some data
    @param aMessage descriptor with data
*/
void TMD5::Update(const TDesC8& aMessage)
{
    Md5_append((const TUint8*)aMessage.Ptr(), aMessage.Length());
}

/**
    Finalise MD5 calculation
    @param  aMessage descriptor with data
    @return pointer to 16-byte array with MD5 hash
*/
TPtrC8 TMD5::Final(const TDesC8& aMessage)
{
    Update(aMessage);
    Md5_finish();
    return TPtrC8(iDigest, HashSize);
}


/**
    Finalise MD5 calculation
    @return pointer to 16-byte array with MD5 hash
*/
TPtrC8 TMD5::Final()
{
    Md5_finish();
    return TPtrC8(iDigest, HashSize);
}






















