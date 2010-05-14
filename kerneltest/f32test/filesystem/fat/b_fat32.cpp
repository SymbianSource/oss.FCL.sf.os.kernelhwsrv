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
// f32test\server\b_fat32.cpp
//
//

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>

#include "fat_utils.h"
#include "t_server.h"

using namespace Fat_Test_Utils;


RTest test(_L("B_FAT32"));

static RRawDisk TheDisk;
static RFile TheFile;
static RDir TheDir;
static TEntry TheEntry;
static TFileName TheFileName;
static TBuf<16> TheDrive;

static HBufC8* pBuffer1=NULL;
static HBufC8* pBuffer2=NULL;
static TBuf8<0x800> TheBuffer;
static TEntry TheFileInfo;
static TVolumeInfo TheVolumeInfo;
static TBuf<8> ThePddName;
static TFatBootSector TheBootSector;

static  TInt64  rndSeed;
static  TFatType gDiskType = EInvalid;

static TInt gFatBits  = 0;
static TInt gBytesPerCluster;
static TInt gEntriesPerCluster;
static TInt gDataStartBytes;
static TInt gRootDirSectors;
static TInt gTotalSectors;
static TInt gRootDirStart;
static TInt gRootSector;
static TInt gRootCluster;
static TInt gFatTestEntries;
static TInt gFatSizeSectors;
static TInt gFirstDataSector;
static TInt gFirstDataCluster;
static TInt gClusterCount;
static TInt gEndOfChain;        // for FAT12/16/32

const TInt KMaxFatEntries  = 2048;
const TInt KMaxFatSize     = KMaxFatEntries * 4;

const TUint KDirAttrReadOnly  = 0x01;
const TUint KDirAttrHidden    = 0x02;
const TUint KDirAttrSystem    = 0x04;
const TUint KDirAttrVolumeId  = 0x08;
const TUint KDirAttrDirectory = 0x10;
const TUint KDirAttrArchive   = 0x20;
const TUint KDirAttrLongName  = KDirAttrReadOnly | KDirAttrHidden | KDirAttrSystem | KDirAttrVolumeId;
const TUint KDirAttrLongMask  = KDirAttrLongName | KDirAttrDirectory | KDirAttrArchive;
const TUint KDirLastLongEntry = 0x40;

void CreateFatEntry(const TDesC& aDir, TBool aVFatEntry, TDes *apFileName=NULL);

#define Error(aMess,aErr)  PutError(__FILE__, __LINE__, aMess,aErr)
static void PutError(const char* aFile, TInt aLine, const TDesC& aMessage,TInt anErr)
    {
    TFileName buf;
    TPtrC8 ptr((const TUint8*)aFile);
    buf.Copy(ptr);
    test.Printf(_L("%S failed - %d\n"), &aMessage,anErr);
    test.Printf(_L("In %S line %d\n"), &buf, aLine);
    test(0);
    }


//
// Position calculation and disk reading routines
// Return number of bytes into the FAT
static  TInt PosInBytes(TInt aFatIndex)
    {
    TInt fatPosInBytes = -1;
    switch (gDiskType)
        {
        case EFat32:
            fatPosInBytes=aFatIndex<<2;
            break;
        case EFat16:
            fatPosInBytes=aFatIndex<<1;
            break;
        case EFat12:
            fatPosInBytes=(aFatIndex*3>>1);
            break;
        default:
            test(0);
        }
    return(fatPosInBytes);
    }

static  TUint32 MaxClusters()
    //
    // Return the number of data clusters on the disk
    //
    {
    TUint32 totSec = (TheBootSector.TotalSectors() ? TheBootSector.TotalSectors() : TheBootSector.HugeSectors());
    TUint32 numSec = totSec - gFirstDataSector;
    return numSec / TheBootSector.SectorsPerCluster();
    }

static  TInt ClusterToByte(TInt aCluster)
    //
    // converts cluster number to byte offset on disk
    //
    {
    if (aCluster < 2)
        return gRootDirStart;
    TInt sector = (aCluster - 2) * gBytesPerCluster + gFirstDataSector * TheBootSector.BytesPerSector();
    return sector;
    }

 TUint32 GetFatEntry(TUint32 aIndex, const TUint8* aFat=NULL)
//
// Read a single FAT entry from disk or FAT copy and return it
//
    {
    TInt pos = PosInBytes(aIndex);

    TUint8  data[4];
    TUint8* ptr = data;

    if (aFat)
        ptr = (TUint8*)aFat + pos;
    else
        {
        pos += TheBootSector.ReservedSectors() * TheBootSector.BytesPerSector();
        TInt r=TheDisk.Open(TheFs,gSessionPath[0]-'A');
        test_KErrNone(r);
        TPtr8 buf(&data[0], 4);
        r=TheDisk.Read(pos, buf);
        test_KErrNone(r);
        TheDisk.Close();
        }

    TUint32 val = 0;
    switch (gDiskType)
        {
        case EFat32:
            val = *(TUint32*)ptr;
            break;
        case EFat16:
            val = *(TUint16*)ptr;
            break;
        case EFat12:
            val = *(TUint16*)ptr;
            if (aIndex & 1)
                val >>= 4;
            val &= 0xFFF;
            break;
        default:
            test(0);
        }
    return val;
    }

 void MarkFatEntry(TUint32 aIndex)
//
// Marks a single FAT entry by modifying it's top 4 bits to
//
    {
    TInt pos = PosInBytes(aIndex);
    pos += TheBootSector.ReservedSectors() * TheBootSector.BytesPerSector();

    TInt r=TheDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);
    TUint8  data[4];
    TPtr8 buf(&data[0], 4);
    r=TheDisk.Read(pos, buf);
    test_KErrNone(r);
    data[3] &= 0x0F;
    data[3] |= 0xA0;
    r=TheDisk.Write(pos, buf);
    test_KErrNone(r);
    TheDisk.Close();
        }

 void DumpBootSector()
//
// Display (in log) TFatBootSector structure
//
    {
    RDebug::Print(_L("BytesPerSector    = %8d"), TheBootSector.BytesPerSector());
    RDebug::Print(_L("SectorsPerCluster = %8d (%d bytes)"),
                  TheBootSector.SectorsPerCluster(), gBytesPerCluster);
    RDebug::Print(_L("ReservedSectors   = %8d"), TheBootSector.ReservedSectors());
    RDebug::Print(_L("NumberOfFats      = %8d"), TheBootSector.NumberOfFats());
    RDebug::Print(_L("RootDirEntries    = %8d"), TheBootSector.RootDirEntries());
    RDebug::Print(_L("TotalSectors      = %8d"), TheBootSector.TotalSectors());
    RDebug::Print(_L("MediaDescriptor   = %8d"), TheBootSector.MediaDescriptor());
    RDebug::Print(_L("FatSectors        = %8d"), TheBootSector.FatSectors());
    RDebug::Print(_L("SectorsPerTrack   = %8d"), TheBootSector.SectorsPerTrack());
    RDebug::Print(_L("NumberOfHeads     = %8d"), TheBootSector.NumberOfHeads());
    RDebug::Print(_L("HiddenSectors     = %8d"), TheBootSector.HiddenSectors());
    RDebug::Print(_L("HugeSectors       = %8d"), TheBootSector.HugeSectors());

    //New for FAT32

    if(TheBootSector.RootDirEntries() == 0) //indicates we have FAT32 volume
        {
        RDebug::Print(_L("FatSectors32      = %8d"), TheBootSector.FatSectors32());
        RDebug::Print(_L("FATFlags          = %8d"), TheBootSector.FATFlags());
        RDebug::Print(_L("VersionNumber     = %8d"), TheBootSector.VersionNumber());
        RDebug::Print(_L("RootClusterNum    = %8d (0x%08X)"),
                      TheBootSector.RootClusterNum(),
                      gRootDirStart);
        RDebug::Print(_L("FSInfoSectorNum   = %8d (0x%08X)"),
                      TheBootSector.FSInfoSectorNum(),
                      TheBootSector.FSInfoSectorNum() * TheBootSector.BytesPerSector());
        RDebug::Print(_L("BkBootRecSector   = %8d (0x%08X)"),
                      TheBootSector.BkBootRecSector(),
                      TheBootSector.BkBootRecSector() * TheBootSector.BytesPerSector());
        }

    TInt fatEntries = gFatSizeSectors*TheBootSector.BytesPerSector();
    switch (gDiskType)
    {
    case EFat32:
        fatEntries /= 4;
        break;
    case EFat16:
        fatEntries /= 2;
        break;
    case EFat12:
        fatEntries *= 3;
        fatEntries /= 2;
        break;
    default:
        test(0);
    }

    RDebug::Print(_L("ClusterCount      = %8d (%ld bytes)"), gClusterCount, ((TInt64)gClusterCount)*gBytesPerCluster);
    RDebug::Print(_L("FatEntries        = %8d (%d sectors)"), fatEntries, gFatSizeSectors);
    RDebug::Print(_L("RootSector        = %8d (0x%08X)"), gRootSector, gRootDirStart);
    RDebug::Print(_L("FirstDataSector   = %8d (0x%08X)"), gFirstDataSector, gDataStartBytes);
    }

 void DumpFat(const TUint8* aFat=NULL)
//
// Dump to the log all those FAT entries which are non-zero
//
    {
    TInt32 max = MaxClusters();
    if (max > KMaxFatEntries)
        max = KMaxFatEntries;
    RDebug::Print(_L("---------------- DUMP OF FAT ---------------"));
    for (TInt32 i = 0; i < max; i++)
        {
        TInt32 val = GetFatEntry(i, aFat);
        TInt32 msk = 0x0FFFFFFF;
        switch (gDiskType)
            {
            case EFat32:
                msk = 0x0FFFFFFF;
                break;
            case EFat16:
                msk = 0xFFFF;
                break;
            case EFat12:
                msk = 0x0FFF;
                break;
            default:
                test(0);
            }
        if ((val & msk) == (0x0FFFFFFF & msk))
            RDebug::Print(_L("    %8d -> EOC"), i);
        else if ((val & msk) == (0x0FFFFFF8 & msk))
            RDebug::Print(_L("    %8d -> Media"), i);
        else if ((val & msk) == (0x0FFFFFF7 & msk))
            RDebug::Print(_L("    %8d -> BAD"), i);
        else if (val > max)
            RDebug::Print(_L("    %8d -> 0x%08X"), i, val);
        else if (val != 0)
            RDebug::Print(_L("    %8d -> %d"), i, val);
        }
    RDebug::Print(_L("--------------------------------------------"));
    }

 TDes* DirAttributes(TInt aAttrib)
//
// Return a pointer to a local buffer containing the attribute letters.
//
    {
    static TBuf<6> str(_L("------"));
    static char*   atr = "RHSVDA";
    for (TInt i = 0; i < 6; i++)
        if ((aAttrib >> i) & 1)
            str[i] = atr[i];
    return &str;
    }

 TBool IsValidDirChar(TUint8 aChar, TUint8 aMin=0x20)
//
// Test whether a character is valid as part of a short filename, aMin is to
// distinguish between first character (which can't be space) and later ones
// which can include space but nothing less.  Note that E5 is a valid character
// in any position, even though it means 'erased' in the first character.
//
    {
    const TUint8* inval = (TUint8*)"\x22\x2A\x2B\x2C\x2F\x3A\x3B\x3C\x3D\x3E\x3F\x5B\x5C\x5D\x7C";
    if (aChar < aMin)
        return EFalse;
    for (const TUint8* p = inval; *p; p++)
        if (aChar == *p)
            return EFalse;
    return ETrue;
    }

 TBool IsValidDirEntry(TFatDirEntry* aDir)
//
// Test whether buffer is a valid normal directory entry
//
    {
    // top two bits of attributes must be zero
    if (aDir->iData[11] & 0xC0)
        return EFalse;
    // first character must be 0x05 or greater than space
    if (!IsValidDirChar(aDir->iData[0], 0x21) && aDir->iData[0] != 0x05)
        return EFalse;
    // other characters in name must be not less than space
    for (TInt i = 1; i < 11; i++)
        if (!IsValidDirChar(aDir->iData[i]))
            return EFalse;
    return ETrue;
    }

 void GetLongNamePart(TDes16& aName, const TUint8* aEntry, TInt aPos, TInt aOffset, TInt aLength)
//
// Extract part of a long name entry into the name buffer.
//
// @param aName   buffer to put name
// @param aEntry  directory entry raw data
// @param aPos    character in buffer to start name segment
// @param aOffset offset in directory entry of the segment
// @param aLength number of characters in the segment
//
    {
    for (TInt i = 0; i < aLength; i++)
        {
        TInt at = i * 2 + aOffset;
        TInt ch = aEntry[at] + aEntry[at+1] * 256;
        aName[aPos++] = TText(ch);
        }
    }

 void ExtractNameString(TDes16& aName, const TUint8* aEntry)
//
// Extract a long name part from a directory entry, truncate it at the first
// NUL (0) character and put quotes round it.
//
    {
    aName.SetLength(15);
    TInt len = aName.Length() - 1;
    TText qu = '\'';
    aName[0] = qu;
    GetLongNamePart(aName, aEntry,  1,  1, 5);
    GetLongNamePart(aName, aEntry,  6, 14, 6);
    GetLongNamePart(aName, aEntry, 12, 28, 2);
    TInt i;
    for (i = 0; i < len; i++)
        if (aName[i] == 0)
            break;
    aName[i++] = qu;
    aName.SetLength(i);
    }

 TBool DumpDirEntry(TInt aNum, const TUint8* aEntry)
//
// Dump a single directory entry to the log.  Return false if it was end of
// directory or an invalid entry (and don't display it).
//
    {
    TFatDirEntry* d = (TFatDirEntry*)aEntry;
    if (d->IsErased())
        {
        // RDebug::Print(_L("%5d: ERASED"), aNum);
        }
    else if (d->IsEndOfDirectory())
        {
        RDebug::Print(_L("%5d: END-OF-DIRECTORY"), aNum);
        return EFalse;
        }
    else if ((d->Attributes() & KDirAttrLongMask) == KDirAttrLongName)
        {
        TBuf16<15> name;
        ExtractNameString(name, aEntry);
        TInt ord = aEntry[0];
        if (ord & KDirLastLongEntry)
            RDebug::Print(_L("%5d: %-15S #%-2d LAST"), aNum, &name, ord & ~KDirLastLongEntry);
        else
            RDebug::Print(_L("%5d: %-15S #%-2d"), aNum, &name, ord & ~KDirLastLongEntry);
        }
    else if (!IsValidDirEntry(d))
        {
        RDebug::Print(_L("%5d: not valid"), aNum);
        return EFalse;
        }
    else
        {
        TBuf<11> name;
        name.Copy(d->Name());
        RDebug::Print(_L("%5d: '%S'  %S  cluster %d"),
                      aNum, &name, DirAttributes(d->Attributes()), d->StartCluster());
        }
    return ETrue;
    }

 void DumpDirCluster(const TUint8* aData, TInt aCluster=0)
//
// Dump directory entries until end of cluster or invalid/end entry found.
//
    {
    if (aCluster > 2)
        aData += (aCluster-2) * gBytesPerCluster;
    for (TInt i = 0; i < gBytesPerCluster; i += KSizeOfFatDirEntry)
        {
        if (DumpDirEntry(i/KSizeOfFatDirEntry, aData))
            aData += KSizeOfFatDirEntry;
        else
            break;
        }
    }

 void DumpData(const TUint8* aFat, TInt aStart, TInt aEnd=-1)
//
// Dump clusters from disk (allows dumping of clusters not in our buffers).
// Only look at clusters marked as 'used' in the FAT.  Note that if aFat is
// NULL the FAT entries will also be read from disk (slower but allows for ones
// outside our copy in memory).
//
    {
    if (aStart > gFatTestEntries)
        return;
    if (aEnd > gFatTestEntries)
        aEnd = gFatTestEntries;
    if (aEnd <= 0)
        aEnd = aStart + 1;
    RDebug::Print(_L("--------------- DATA AREA ------------------"));
    if (aEnd > gFatTestEntries)
        aEnd = gFatTestEntries;
    for (TInt cluster = aStart; cluster < aEnd; cluster++)
        {
        if (GetFatEntry(cluster, aFat) != 0)
            {
            HBufC8* buf=HBufC8::New(gBytesPerCluster);
            test(buf!=NULL);
            TPtr8 ptr=buf->Des();
            TInt r=TheDisk.Open(TheFs,gSessionPath[0]-'A');
            test_KErrNone(r);
            r=TheDisk.Read(ClusterToByte(cluster), ptr);
            test_KErrNone(r);
            TheDisk.Close();
            RDebug::Print(_L("Cluster %d @ 0x%08X:"), cluster, ClusterToByte(cluster));
            DumpDirCluster(ptr.Ptr());
            delete buf;
            }
        }
    RDebug::Print(_L("--------------------------------------------"));
    }

 void DumpData(TInt aStart=0, TInt aEnd=0)
//
// Dump clusters from disk (allows dumping of clusters not in our buffers).
// Only look at clusters marked as 'used' in the FAT.  Note that if aFat is
// NULL the FAT entries will also be read from disk (slower but allows for ones
// outside our copy in memory).
//
    {
    if (aStart == 0)
        {
        if (aEnd <= 0)
            aEnd = 1;
        TInt num = (gDiskType == EFat32 ? aEnd*gEntriesPerCluster : TheBootSector.RootDirEntries());
        TInt pos = gRootDirStart;
        TInt ent = 0;
        HBufC8* buf=HBufC8::New(KSizeOfFatDirEntry);
        test(buf!=NULL);
        TPtr8 ptr=buf->Des();
        TInt r=TheDisk.Open(TheFs,gSessionPath[0]-'A');
        test_KErrNone(r);
        RDebug::Print(_L("--------------- ROOT DIR ------------------"));
        for (TInt i = 0; i < num; i++)
            {
            r=TheDisk.Read(pos, ptr);
            test_KErrNone(r);
            if (!DumpDirEntry(ent, ptr.Ptr()))
                break;
            pos += KSizeOfFatDirEntry;
            }
        RDebug::Print(_L("-------------------------------------------"));
        TheDisk.Close();
        delete buf;
        }
    else if (aStart == 1)
        {
        DumpData(0, 1);
        DumpData(NULL, gFirstDataCluster, aEnd);
        }
    else
        {
        DumpData(NULL, aStart, aEnd);
        }
    }

 void DumpHex(const TUint8* aData, TInt aLen)
//
// Dump a block of memory to the log in hex.
//
    {
    for (TInt base = 0; base < aLen; base += 16)
        {
        TBuf<16*3> buf;
        TInt off;
        for (off = base; off < aLen && off < base + 16; off++)
            {
            buf.Append(TText(' '));
            buf.AppendNumFixedWidth(aData[off], EHex, 2);
            }
        RDebug::Print(_L("%04X: %S"), base, &buf);
        }
    }


//---------------------------------------------------------------------------------------------------------------

static void DoReadBootSector(TFatBootSector& aBootSector)
{
    TInt nRes = ReadBootSector(TheFs, CurrentDrive(), KBootSectorNum<<KDefaultSectorLog2, aBootSector);
    test(nRes == KErrNone);

    if(!aBootSector.IsValid())
        {
        test.Printf(_L("Wrong bootsector! Dump:\n"));
        aBootSector.PrintDebugInfo();
        test(0);
        }

    // Calculate derived variables (fixed for a particular disk format)

    if (TheBootSector.FatType() == EFat32)
        {
        gDiskType = EFat32;
        gFatBits  = 32;
        gEndOfChain = 0x0FFFFFFF;
        }
    else if (TheBootSector.FatType() == EFat16)
        {
        gDiskType = EFat16;
        gFatBits  = 16;
        gEndOfChain = 0xFFFF;
        }
    else
        {
        gDiskType = EFat12;
        gFatBits  = 12;
        gEndOfChain = 0x0FFF;
        }

    gBytesPerCluster   = TheBootSector.BytesPerSector() * TheBootSector.SectorsPerCluster();
    gRootDirSectors    = ((TheBootSector.RootDirEntries() * KSizeOfFatDirEntry + TheBootSector.BytesPerSector() - 1) /
                          TheBootSector.BytesPerSector());
    gEntriesPerCluster = gBytesPerCluster / KSizeOfFatDirEntry;
    gTotalSectors      = (TheBootSector.TotalSectors() ? TheBootSector.TotalSectors() : TheBootSector.HugeSectors());

    switch (gDiskType)
        {
        case EFat12:
        case EFat16:
            gFatSizeSectors   = TheBootSector.FatSectors();
            gRootSector       = TheBootSector.ReservedSectors() + TheBootSector.NumberOfFats() * gFatSizeSectors;
            gFirstDataSector  = gRootSector + gRootDirSectors;
            gRootCluster      = 0;
            gFirstDataCluster = 2;
            gDataStartBytes   = gFirstDataSector * TheBootSector.BytesPerSector();
            gRootDirStart     = gRootSector * TheBootSector.BytesPerSector();
            break;
        case EFat32:
            gFatSizeSectors   = TheBootSector.FatSectors32();
            gRootSector       = TheBootSector.ReservedSectors() + TheBootSector.NumberOfFats() * gFatSizeSectors;
            gFirstDataSector  = gRootSector + gRootDirSectors;
            gRootCluster      = 2;
            gFirstDataCluster = 3;
            gDataStartBytes   = gFirstDataSector * TheBootSector.BytesPerSector();
            gRootDirStart     = (TheBootSector.RootClusterNum() - 2) * gBytesPerCluster + gDataStartBytes;
            break;
        default:
            break;
        }

    gClusterCount   = (gTotalSectors - gFirstDataSector) / TheBootSector.SectorsPerCluster();

    gFatTestEntries = MaxClusters();
    if (gFatTestEntries > KMaxFatSize)
        gFatTestEntries = KMaxFatSize;
    }


static  TInt CalcShifts(TInt aSize)
//
// Calculate the number of shifts to get >= aSize (aSize should be a power of 2
// anyway).
//
    {
    TInt x=0;
    while (aSize>>=1)
        x++;
    return(x);
    }

static  TInt SectorShifts()
//
// Calculate number of shifts for sector size.
//
    {
    return(CalcShifts(TheBootSector.BytesPerSector()));
    }

static  TInt ClusterShifts()
//
// Calculate number of shifts for cluster size.
//
    {
    return(CalcShifts(TheBootSector.BytesPerSector()*TheBootSector.SectorsPerCluster()));
    }


//
// Quick Format the disk
//
static void FormatPack()
    {

    #if 0
    //-- FAT32 SPC:1; for the FAT32 testing on the emulator
    TFatFormatParam fp;
    fp.iFatType = EFat32;
    fp.iSecPerCluster = 1;
    FormatFatDrive(TheFs, CurrentDrive(), ETrue, &fp);
    #else

    FormatFatDrive(TheFs, CurrentDrive(), ETrue);

    #endif

    DoReadBootSector(TheBootSector);

    }



static  void TestReadWrite(TInt64 aPos,TInt aLen,TInt anErr)
//
// Read and write to the disk
//
    {
    TPtr8 buffer((TUint8*)pBuffer1->Ptr(),aLen);
    test.Printf(_L("TestReadWrite pos=0x%lx,len=%d\n"),aPos,aLen);
    TInt r;
    if ((r=TheDisk.Read(aPos,buffer))!=anErr)
        {
        test.Printf(_L("ERROR: anErr=%d ret=%d\n"),anErr,r);
        test(EFalse);
        }
    buffer.SetLength(aLen);
    if ((r=TheDisk.Write(aPos,buffer))!=anErr)
        {
        test.Printf(_L("ERROR: anErr=%d ret=%d\n"),anErr,r);
        test(EFalse);
        }
    }

static  TInt ReadWriteWord(TInt64 aPos,TInt aMask,TInt aValue)
//
// Read 2 bytes from aPos and Write over masked bits with aValue
//
    {
    TUint16 word;
    TPtr8 buffer((TUint8*)&word,sizeof(word));

    TInt r=TheDisk.Read(aPos,buffer);
    if (r!=KErrNone)
        return(r);

    word&=((aValue&aMask)|~aMask);
    word|=(aValue&aMask);

    r=TheDisk.Write(aPos,buffer);
    return(r);
    }

static  TInt ReadWriteDWord(TInt64 aPos,TInt aMask,TInt aValue)
//
// Read 4 bytes from aPos and Write over masked bits with aValue
//
    {
    TUint32 word;
    TPtr8 buffer((TUint8*)&word,sizeof(word));

    TInt r=TheDisk.Read(aPos,buffer);
    if (r!=KErrNone)
        return(r);

    word&=((aValue&aMask)|~aMask);
    word|=(aValue&aMask);

    r=TheDisk.Write(aPos,buffer);
    return(r);
    }

static  void FatWrite(TInt aCluster,TInt aValue)
//
//
//
    {
    TInt pos=0;
    TInt mask=0;

    const TUint32  KFirstFatSectorPos = TheBootSector.FirstFatSector() * TheBootSector.BytesPerSector();

    switch (gDiskType)
        {
        case EFat32:
            mask=0xffffffff;
            pos=KFirstFatSectorPos+(aCluster<<2);
            break;
        case EFat16:
            mask=0xffff;
            pos=KFirstFatSectorPos+(aCluster<<1);
            break;
        case EFat12:
            mask=0x0fff;
            pos=KFirstFatSectorPos+aCluster+(aCluster>>1);
            if (aCluster & 1)
                {
                mask=0xfff0;
                aValue<<=4;
                }
            break;
        default:
            test(0);
        }

    TInt r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);
    test(ReadWriteDWord(pos,mask,aValue)==KErrNone);
    TheDisk.Close();
    }

static  void TestRwWord(TInt64 aPos,TInt anErr)
//
//
//
    {
    TInt r;
    TUint16 wBuf;
    TUint16 rBuf;
    TUint16 mask=0;
    TUint16 value=0;

    test.Printf(_L("Test read and write value to 0x%lx\n"),aPos);

    if ((r=ReadWriteWord(aPos,mask,value))!=anErr)
        {
        test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
        test(EFalse);
        }

    if (anErr==KErrNone && aPos==0)
        {
        wBuf=0xff00;
        TPtrC8 writebuf((TUint8*)&wBuf,sizeof(wBuf));
        test(TheDisk.Write(aPos,writebuf)==KErrNone);

        mask=0x0505;
        value=0xa4a4;
        test.Printf(_L("Test RWW mask=%04x value%04x\n"),mask,value);
        if ((r=ReadWriteWord(aPos,mask,value))!=anErr)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }

        TPtr8 readBuf((TUint8*)&rBuf,sizeof(rBuf));
        if ((r=TheDisk.Read(aPos,readBuf))!=KErrNone)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }
        test(rBuf==0xfe04);
        }

    if (anErr==KErrNone && aPos==1)
        {
        wBuf=0xff00;
        TPtrC8 writebuf((TUint8*)&wBuf,sizeof(wBuf));
        test(TheDisk.Write(aPos,writebuf)==KErrNone);

        mask=0xffff;
        value=0xa3a3;
        test.Printf(_L("Test RWW mask=%04x value%04x\n"),mask,value);
        if ((r=ReadWriteWord(aPos,mask,value))!=anErr)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }

        TPtr8 readBuf((TUint8*)&rBuf,sizeof(rBuf));
        if ((r=TheDisk.Read(aPos,readBuf))!=KErrNone)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }
        test(rBuf==0xa3a3);
        }
    }

static  void TestRwDWord(TInt64 aPos,TInt anErr)
//
//
//
    {
    TInt r;
    TUint32 wBuf;
    TUint32 rBuf;
    TUint32 mask=0;
    TUint32 value=0;

    test.Printf(_L("Test read and write value to 0x%lx\n"),aPos);

    if ((r=ReadWriteDWord(aPos,mask,value))!=anErr)
        {
        test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
        test(EFalse);
        }

    if (anErr==KErrNone && aPos==0)
        {
        wBuf=0xff00ff00;
        TPtrC8 writebuf((TUint8*)&wBuf,sizeof(wBuf));
        test(TheDisk.Write(aPos,writebuf)==KErrNone);

        mask  = 0x0505195c;
        value = 0xa4a4c634;
        test.Printf(_L("Test RWW mask=%04x value%04x\n"),mask,value);
        if ((r=ReadWriteDWord(aPos,mask,value))!=anErr)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }

        TPtr8 readBuf((TUint8*)&rBuf,sizeof(rBuf));
        if ((r=TheDisk.Read(aPos,readBuf))!=KErrNone)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }
        test(rBuf==0xfe04e614);
        }

    if (anErr==KErrNone && aPos==1)
        {
        wBuf=0xff0000ff;
        TPtrC8 writebuf((TUint8*)&wBuf,sizeof(wBuf));
        test(TheDisk.Write(aPos,writebuf)==KErrNone);

        mask=0xffffffff;
        value=0xa3a3dead;
        test.Printf(_L("Test RWW mask=%04x value%04x\n"),mask,value);
        if ((r=ReadWriteDWord(aPos,mask,value))!=anErr)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }

        TPtr8 readBuf((TUint8*)&rBuf,sizeof(rBuf));
        if ((r=TheDisk.Read(aPos,readBuf))!=KErrNone)
            {
            test.Printf(_L("ERROR: anErr=%d, ret=%d\n"),anErr,r);
            test(EFalse);
            }
        test(rBuf==0xa3a3dead);
        }
    }


static  TInt ThrottleDirEntries(TInt aDirEntries, TInt aRemainder)
    {
    // throttle the number of entries needed, since for large cluster
    // sizes, this can take forever (eg 2GB card -> a cluster size of 32K
    // -> 1024 entries per cluster
    const TInt KMaxDirEntries = 2048;
    test(aRemainder < KMaxDirEntries);
    TInt maxDirEntries = KMaxDirEntries - aRemainder;

    if (aDirEntries > maxDirEntries)
        {
        RDebug::Print(_L("Reducing directory entries from %d to %d"), aDirEntries, maxDirEntries);
        aDirEntries = maxDirEntries;
        }

    return aDirEntries;
    }

static  void TestLoopedSubDir()
//
//
    {
    test.Printf(_L("Test looped sub-dir\n"));
    FormatPack();
    TInt r=TheFs.MkDir(_L("\\D\\"));
    if (r!=KErrNone && r!=KErrAlreadyExists)
        Error(_L("Failed to make directory"),r);
    TheFileName=_L("\\D\\");

    TInt i=0;
    TInt dirEntriesNeeded = ((TheBootSector.BytesPerSector()*TheBootSector.SectorsPerCluster()/KSizeOfFatDirEntry)-2);
    dirEntriesNeeded = ThrottleDirEntries(dirEntriesNeeded, 2);


    //-- generate some number of VFAT dir. entries by creating  8.3 temp. files in a lower case
    for (i=0;i<dirEntriesNeeded;i++)
        {
        CreateFatEntry(TheFileName, ETrue);
        }

    test.Printf(_L("Test dir with no match\n"));
    FatWrite(gFirstDataCluster,gFirstDataCluster);
    if ((r=TheDir.Open(TheFs,_L("\\D\\nomatch"),KEntryAttMaskSupported))!=KErrNone)
        Error(_L("Failed Directory open"),r);
    if ((r=TheDir.Read(TheEntry))!=KErrCorrupt)
        Error(_L("Failed Directory read"),r);
    TheDir.Close();

    test.Printf(_L("Test dir with match\n"));
    if ((r=TheDir.Open(TheFs,_L("\\D\\*.*"),KEntryAttMaskSupported))!=KErrNone)
        Error(_L("Failed Directory open"),r);
    if ((r=TheDir.Read(TheEntry))!=KErrNone)
        Error(_L("Failed Directory read"),r);
    TheDir.Close();

    test.Printf(_L("Test dir without loop\n"));
    FatWrite(gFirstDataCluster,gEndOfChain);
    if ((r=TheDir.Open(TheFs,_L("\\D\\nomatch"),KEntryAttMaskSupported))!=KErrNone)
        Error(_L("Directory open"),r);
    if ((r=TheDir.Read(TheEntry))!=KErrEof)
        Error(_L("Reading empty dir returned"),r);
    TheDir.Close();

    test.Printf(_L("Test dir with long filenames\n"));

    FormatPack();
    r=TheFs.MkDir(_L("\\D\\"));
    if (r!=KErrNone && r!=KErrAlreadyExists)
        Error(_L("Failed to make directory"),r);
    TheFileName=_L("\\D\\");

    dirEntriesNeeded = ((TheBootSector.BytesPerSector()*TheBootSector.SectorsPerCluster()/KSizeOfFatDirEntry)-3);
    dirEntriesNeeded = ThrottleDirEntries(dirEntriesNeeded, 3);

    //-- generate some number of VFAT dir. entries by creating  8.3 temp. files in a lower case
    for (i=0;i<dirEntriesNeeded;i++)
        {
        CreateFatEntry(TheFileName, ETrue);
        }

    MakeFile(_L("\\D\\longfileName.Long"));

    test.Printf(_L("Test dir with no match\n"));
    FatWrite(gFirstDataCluster,gFirstDataCluster);
    if ((r=TheDir.Open(TheFs,_L("\\D\\nomatch"),KEntryAttMaskSupported))!=KErrNone)
        Error(_L("Failed Directory open"),r);
    if ((r=TheDir.Read(TheEntry))!=KErrCorrupt)
        Error(_L("Failed Directory read"),r);
    TheDir.Close();

    test.Printf(_L("Test dir with match\n"));
    if ((r=TheDir.Open(TheFs,_L("\\D\\*.*"),KEntryAttMaskSupported))!=KErrNone)
        Error(_L("Failed Directory open"),r);
    if ((r=TheDir.Read(TheEntry))!=KErrNone)
        Error(_L("Failed Directory read"),r);
    TheDir.Close();

    test.Printf(_L("Test dir without loop\n"));
    FatWrite(gFirstDataCluster,gEndOfChain);
    if ((r=TheDir.Open(TheFs,_L("\\D\\nomatch"),KEntryAttMaskSupported))!=KErrNone)
        Error(_L("Directory open"),r);

#if !defined _UNICODE
    if ((r=TheDir.Read(TheEntry))!=KErrCorrupt)
        Error(_L("Reading empty dir returned"),r);
#endif
    TheDir.Close();
    }

static  void TestLoopedFile()
//
// Test Looped file
//
    {
    test.Printf(_L("Test looped file\n"));
    FormatPack();
    TInt r;



    test.Next(_L("CreateFile"));
    test(TheFile.Replace(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite)==KErrNone);
    TPtr8 buf=pBuffer1->Des();

    test(TheFile.Write(buf,TheBootSector.BytesPerSector()-1)==KErrNone);
    TheFile.Close();

    test.Next(_L("Write 1 cluster loop"));
    FatWrite(gFirstDataCluster,gFirstDataCluster);              /* tiny loop */
    if ((r=TheFile.Open(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite))!=KErrCorrupt)
        Error(_L("Error opening corrupt file"),r);
    FatWrite(gFirstDataCluster,0);
    if ((r=TheFile.Open(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite))!=KErrCorrupt)
        Error(_L("Error opening corrupt file"),r);
    FatWrite(gFirstDataCluster,gEndOfChain);
    if ((r=TheFile.Open(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite))!=KErrNone)
        Error(_L("Error opening file"),r);
    if ((r=TheFile.Write(buf,TheBootSector.BytesPerSector()*TheBootSector.SectorsPerCluster()*2-1))!=0)
        Error(_L("Error writing to file"),r);
    TheFile.Close();

    test.Next(_L("Write 2 cluster loop"));
    FatWrite(gFirstDataCluster+1,gFirstDataCluster);             /* 2 cluster loop */
    if ((r=TheFile.Open(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite))!=KErrCorrupt)
        Error(_L("Error opening corrupt file"),r);
    FatWrite(gFirstDataCluster+1,gEndOfChain);
    if ((r=TheFile.Open(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite))!=KErrNone)
        Error(_L("Error opening file"),r);

    TInt len=16384;
    TInt size=0L;
    while (size < gBytesPerCluster * 500)
        {
        test.Printf(_L("\rWriting %d      "),size);
        if ((r=TheFile.Write(buf,len))!=KErrNone)
            {
            if (r!=KErrDiskFull)
                Error(_L("File write error"),r);
            len>>=1;
            if (len==0)
                break;
            }
        else
        size+=len;
        }
    test.Printf(_L("\n"));
    TheFile.Close();

    RDebug::Print(_L("File created size %d"), size);
    TInt clust=((size-1)>>ClusterShifts())+gFirstDataCluster;
    FatWrite(clust,gFirstDataCluster);
    if ((r=TheFile.Open(TheFs,_L("\\LOOPED1.TMP"),EFileRead|EFileWrite))!=KErrCorrupt)
        Error(_L("Error opening corrupt file"),r);
    FatWrite(clust,gEndOfChain);
    if ((r=TheFs.Delete(_L("\\LOOPED1.TMP")))!=KErrNone)
        Error(_L("Error deleting file"),r);
    RDebug::Print(_L("File removed"));
    r=TheFs.CheckDisk(gSessionPath);
    test_KErrNone(r);
    }

static  void TestFatEntry(TUint16 aFileSize,TInt aCorruptFatCluster)
//
// Test fat entry
//
    {
    TInt r;
    test.Printf(_L("File size=%d, cluster value=0x%x\n"),aFileSize,aCorruptFatCluster);
    FormatPack();

    r=TheFile.Replace(TheFs,_L("\\CORRUPT2.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    TheBuffer.SetLength(aFileSize);
    Mem::Fill(&TheBuffer[0],aFileSize,'A');
    r=TheFile.Write(TheBuffer);
    test_KErrNone(r);
    TheFile.Close();

    FatWrite(gFirstDataCluster,aCorruptFatCluster);

    TInt pos=0;
    r=TheFile.Open(TheFs,_L("\\CORRUPT2.TMP"),EFileRead|EFileWrite);
    test_Value(r, r == KErrNone || r==KErrCorrupt);
    if (r==KErrNone)
        {
        r=TheFile.Seek(ESeekStart,pos);
        test_KErrNone(r);
        r=TheFile.Write(TheBuffer);

        if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
            r = TheFile.Flush();

        if (r != KErrCorrupt)
            {
            test.Printf(_L("Predicted error %d Actual error %d\n"),KErrCorrupt,r);
            Error(_L("Failed write"),r);
            }
        TheFile.Close();
        }

    FatWrite(gFirstDataCluster,gEndOfChain);

    pos=0;
    r=TheFile.Open(TheFs,_L("\\CORRUPT2.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    r=TheFile.Seek(ESeekStart,pos);
    test_KErrNone(r);
    r=TheFile.Write(TheBuffer);

    if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
            r = TheFile.Flush();

    // if the file size <= cluster size then writing last cluster marker to
    // cluster 2 should have no effect
    if(aFileSize>TheBootSector.SectorsPerCluster()<<SectorShifts())
        {
        if (r!=KErrCorrupt)
            {
            test.Printf(_L("Predicted error %d Actual error %d\n"),KErrCorrupt,r);
            Error(_L("Failed write"),r);
            }
        }
    else
        {
        if (r!=KErrNone)
            {
            test.Printf(_L("Predicted error %d Actual error %d\n"),KErrNone,r);
            Error(_L("Failed write"),r);
            }
        }
    TheFile.Close();
    }

static  void TestDirEntry(TInt anInitialSize,TInt aWriteLen,TInt aCorruptStartCluster)
//
// Test directory entry
//
    {
    test.Printf(_L("Initial size=%d, len=%d, start cluster=0x%x\n"),anInitialSize,aWriteLen,aCorruptStartCluster);
    FormatPack();
    TInt r;

    test(TheFile.Create(TheFs,_L("\\CORRUPT1.TMP"),EFileRead|EFileWrite)==KErrNone);
    TheBuffer.SetLength(anInitialSize);
    Mem::Fill(&TheBuffer[0],anInitialSize,'A');
    r=TheFile.Write(TheBuffer);
    test_KErrNone(r);
    TheFile.Close();

    r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);
    TPtr8 sectorBuf((TUint8*)pBuffer1->Ptr(),TheBootSector.BytesPerSector());
    TInt pos = gRootDirStart;
    r=TheDisk.Read(pos,sectorBuf);
    test_KErrNone(r);
    TFatDirEntry* pE=(TFatDirEntry*)pBuffer1->Ptr();
    while (pE->IsVFatEntry())   //  UNICODE entries are VFat by definition
        pE++;

    pE->SetStartCluster(aCorruptStartCluster);
    test(TheDisk.Write(pos,sectorBuf)==KErrNone);


    //-- a small hack to avoid problems with the fact that FAT[1] entry
    //-- is now used for marking volume as clean.  TheDisk.Close() cause volume remout and
    //-- the data
    TheDisk.Close();
    r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);


    pos=0;
    TPtr8 buffer1(pBuffer1->Des());
    r=TheDisk.Read(pos,buffer1);
    test_KErrNone(r);
    TheDisk.Close();
    r=TheFs.Entry(_L("\\CORRUPT1.TMP"),TheEntry);
    test_Value(r, r == KErrNone || r==KErrCorrupt);
    TTime saveTime=TheEntry.iModified;
    if (r!=KErrNone)
        saveTime.HomeTime();

    r=TheFile.Open(TheFs,_L("\\CORRUPT1.TMP"),EFileRead|EFileWrite);
    if (r==KErrNone)
        {
        TheBuffer.SetLength(aWriteLen);
        Mem::Fill(&TheBuffer[0],aWriteLen,'B');
        if ((r=TheFile.Write(TheBuffer))!=KErrCorrupt)
                {
                test.Printf(_L("Predicted error %d Actual error %d\n"),KErrCorrupt,r);
                Error(_L("Failed write"),r);
                }
        TheFile.Close();
        }

    r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);
    pos=0;
    TPtr8 buffer2(pBuffer2->Des());
    r=TheDisk.Read(pos,buffer2);
    test_KErrNone(r);

    //-- this bit is dodgy. The buffers may differ because of volume finalisation stuff
    //-- FAT[1] and FSInfo sectors
    test(buffer1==buffer2);
    TheDisk.Close();

    r=TheFs.SetModified(_L("\\CORRUPT1.TMP"),saveTime);
    test_Value(r, r == KErrNone || r==KErrCorrupt);
    r=TheFs.Entry(_L("\\CORRUPT1.TMP"),TheEntry);
    test_Value(r, r == KErrNone || r==KErrCorrupt);
    }

static  void TestBounds()
//
// Test reading/writing past the end of a drive
//
    {
    test.Next(_L("Test read/write past boundaries"));
    test(TheFs.Volume(TheVolumeInfo,CurrentDrive())==KErrNone);
    TInt64 size=TheVolumeInfo.iSize;
    TInt r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);
    TPtr8 buffer(pBuffer1->Des());
    TInt64 pos=size - 2*buffer.MaxLength();
    TInt inc=buffer.MaxLength();
    FOREVER
        {
        TPtr8 tempbuf((TUint8*)pBuffer1->Ptr(),inc);
        r=TheDisk.Read(pos,tempbuf);
        test.Printf(_L("Read %08X:%08X len %d r %d\r"), I64HIGH(pos),I64LOW(pos), inc, r);
        test_Value(r, r == KErrNone || r==KErrCorrupt);
        if (r==KErrNone)
            pos+=inc;
        else
            {
            inc>>=1;
            if (inc==0)
                break;
            }
        test(pos<2*size);
        }

    TInt64 maxcalc= TInt64(gTotalSectors) * TInt64(TheBootSector.BytesPerSector());

    test.Printf(_L("\n"));
    test.Printf(_L("Volume size = %ld\n"), size);
    test.Printf(_L("RawDiskSize = %ld\n"), maxcalc);
    test.Printf(_L("MaxReadPos  = %ld\n"), pos);

    TInt64 maxpos = pos;

    // check that the calculated raw size of the disk is equal to the MaxReadPos that
    // has just been discovered by trial and error
    test(maxcalc == maxpos);

    for (TInt64 bsize = 1; bsize < 8; bsize++)
        {
        test.Printf(_L("\n"));
        test.Printf(_L("Buffer size %d\n"), bsize);
        for (TInt64 bpos = MAKE_TINT64(0, 0x1000); bpos < MAKE_TINT64(0x3FFFFFFF,0); bpos<<=1)
            {
            TInt64 endPos = (bpos + 1);
            for (TInt64 lpos = bpos - bsize; lpos <= endPos; lpos++)
                {
                TPtr8 temp((TUint8*) (pBuffer1->Ptr()), (TInt) bsize);
                TInt expect = (lpos+bsize-1 < maxpos ? KErrNone : KErrCorrupt);
                r=TheDisk.Read(lpos, temp);
                RDebug::Print(_L("Read %08X:%08X result %d     \r"), I64HIGH(lpos), I64LOW(lpos), r);
                test_Value(r, r == expect);
                }
            }
        }

    RDebug::Print(_L("\n"));

    TestReadWrite(0L,0,0);
    TestReadWrite(0L,1,0);
    TestReadWrite(pos-1,1,0);
    TestReadWrite(pos-0x100,0x100,0);
    TestReadWrite(pos-1,2,KErrCorrupt);
    TestReadWrite(pos-0x100,0x101,KErrCorrupt);
    TestReadWrite(pos-0xff,0x100,KErrCorrupt);
    TestReadWrite(pos,0,0);
    TestReadWrite(pos,1,KErrCorrupt);

    TestReadWrite(pos-16384,16384,0);
    TestReadWrite(pos-16384,16385,KErrCorrupt);

    TInt errVal=(pos>32768+0x100) ? KErrNone : KErrCorrupt;
    TestReadWrite(32768L,0x100,errVal);
    errVal=(pos>32768+0x101) ? KErrNone : KErrCorrupt;
    TestReadWrite(32768L,0x101,errVal);
    errVal=(pos>32768+0x1ff) ? KErrNone : KErrCorrupt;
    TestReadWrite(32768L,0xff,errVal);
    errVal=(pos>65000+0x100) ? KErrNone : KErrCorrupt;
    TestReadWrite(65000L,0x100,errVal);

    errVal=(pos>0x2000000+1) ? KErrNone : KErrCorrupt;
    TestReadWrite(0x2000000L,1,errVal);

    TestRwWord(0L,0);
    TestRwWord(1L,0);
    TestRwWord(pos-2,0);
    TestRwWord(pos-1,KErrCorrupt);
    TestRwWord(pos,KErrCorrupt);
    TestRwWord(pos+1,KErrCorrupt);

    TestRwDWord(0L,0);
    TestRwDWord(1L,0);
    TestRwDWord(2L,0);
    TestRwDWord(3L,0);
    TestRwDWord(pos-4,0);
    TestRwDWord(pos-3,KErrCorrupt);
    TestRwDWord(pos-2,KErrCorrupt);
    TestRwDWord(pos-1,KErrCorrupt);
    TestRwDWord(pos,KErrCorrupt);
    TestRwDWord(pos+1,KErrCorrupt);

    TheDisk.Close();
    }

static  void TestClusters()
    {
    test.Next(_L("Test corrupt start cluster"));
    //          Initial  Write  Corrupt
    //           Size     Len   Cluster
    TestDirEntry(1024,    513,      0);
    TestDirEntry( 512,    512,      0);
    TestDirEntry(1024,    513,      1);
    TestDirEntry( 512,    512,      1);
    TestDirEntry(1024,    513,  0xff0);

    test.Printf(_L("Test corrupt chain\n"));
    TestFatEntry(1536,0);
    TestFatEntry(1536,1);

//  TInt fatCacheSize=FatCacheSize();
//  TUint16 cluster16=(TUint16)(fatCacheSize/2);
//  TUint16 cluster12=(TUint16)((fatCacheSize/3)*2);
//  TestFatEntry(1536,cluster12);
//  TestFatEntry(1536,cluster16);
    TestFatEntry(1536,0xff0);
    // don't test when only one cluster for the file
    if(1536>gBytesPerCluster)
        TestFatEntry(1536,gEndOfChain);

    TestLoopedFile();
    TestLoopedSubDir();
    }


static  void TestClusterAllocation()
//
// Test number of clusters allocated
//
    {
    test.Next(_L("Test number of clusters allocated is correct"));

    FormatPack();

    RFile f;
    TInt r;

    r=f.Replace(TheFs,_L("\\GOBLIN.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    f.SetSize(4*gBytesPerCluster); // 4 Clusters
    f.Close();

    r=f.Replace(TheFs,_L("\\WIZARD.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    f.SetSize(5*gBytesPerCluster); // 5 Clusters
    f.Close();

    r=f.Replace(TheFs,_L("\\TROLL.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    f.SetSize(3*gBytesPerCluster); // 3 Clusters
    f.Close();

    r=f.Replace(TheFs,_L("\\GNOME.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    f.SetSize(10*gBytesPerCluster); // 10 Clusters
    f.Close();

    r=f.Replace(TheFs,_L("\\CYCLOPS.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    f.SetSize(gBytesPerCluster); // 1 Cluster
    f.Close();

    r=f.Replace(TheFs,_L("\\PIXIE.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    f.SetSize(gBytesPerCluster); // 1 Cluster
    f.Close();

    r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);
    TPtr8 sectorBuf((TUint8*)pBuffer1->Ptr(),TheBootSector.BytesPerSector());
    TInt pos = gRootDirStart;
    test(TheDisk.Read(pos,sectorBuf)==KErrNone);
    TheDisk.Close();

    TFatDirEntry* pE=(TFatDirEntry*)pBuffer1->Ptr();
    while (pE->IsVFatEntry())   //  UNICODE 8.3 filenames are VFAT by definition
        pE++;

    TInt cluster=pE->StartCluster();
    TBuf8<15> name=pE->Name();
    test(name==_L8("GOBLIN  TMP"));

    pE++;
    while (pE->IsVFatEntry())
        pE++;

    test((pE->StartCluster()-cluster)==4);
    cluster=pE->StartCluster();
    name=pE->Name();
    test(name==_L8("WIZARD  TMP"));

    pE++;
    while (pE->IsVFatEntry())
        pE++;

    test((pE->StartCluster()-cluster)==5);
    cluster=pE->StartCluster();
    name=pE->Name();
    test(name==_L8("TROLL   TMP"));

    pE++;
    while (pE->IsVFatEntry())
        pE++;

    test((pE->StartCluster()-cluster)==3);
    cluster=pE->StartCluster();
    name=pE->Name();
    test(name==_L8("GNOME   TMP"));

    pE++;
    while (pE->IsVFatEntry())
        pE++;

    test ((pE->StartCluster()-cluster)==10);
    cluster=pE->StartCluster();
    name=pE->Name();
    test(name==_L8("CYCLOPS TMP"));

    pE++;
    while (pE->IsVFatEntry())
        pE++;

    test((pE->StartCluster()-cluster)==1);
    name=pE->Name();
    test(name==_L8("PIXIE   TMP"));

    r=TheFs.Delete(_L("\\GOBLIN.TMP"));
    test_KErrNone(r);
    r=TheFs.Delete(_L("\\WIZARD.TMP"));
    test_KErrNone(r);
    r=TheFs.Delete(_L("\\TROLL.TMP"));
    test_KErrNone(r);
    r=TheFs.Delete(_L("\\GNOME.TMP"));
    test_KErrNone(r);
    r=TheFs.Delete(_L("\\CYCLOPS.TMP"));
    test_KErrNone(r);
    r=TheFs.Delete(_L("\\PIXIE.TMP"));
    test_KErrNone(r);

    FormatPack();

    }


static  void TestMakeDir(const TDesC& aName, TUint aNewClust, TUint aParentClust)
//
// Test make dir
//
    {
    test.Printf(_L("Checking cluster %02d, parent %d: \"%S\"\n"), aNewClust, aParentClust, &aName);

    TInt r=TheFs.MkDir(aName);
    test_Value(r, r == KErrNone || r==KErrAlreadyExists);

    TInt pos=ClusterToByte(aNewClust);
    TPtr8 sectorBuf((TUint8*)pBuffer1->Ptr(),gBytesPerCluster);

    r=TheDisk.Open(TheFs,CurrentDrive());
    if ((r=TheDisk.Read(pos,sectorBuf))!=KErrNone)
        Error(_L("Reading data"),r);
    TheDisk.Close();

    TFatDirEntry* pE=(TFatDirEntry*)pBuffer1->Ptr();
    if (pE->Name()[0]!='.' || pE->Name()[1]!=' ')
        {
        while (pE->IsVFatEntry())
            pE++;
        if (pE->Name()[0]!='.' || pE->Name()[1]!=' ')
            Error(_L("Failed to find '.' entry"),KErrNone);
        }
    if (pE->StartCluster()!=aNewClust)
        Error(_L("Bad directory start cluster"),KErrNone);
    pE++;
    if (pE->Name()[0]!='.' || pE->Name()[1]!='.')
        Error(_L("Second entry is not '..'"),KErrNone);
    if (pE->StartCluster() != ((aParentClust==(TUint)gRootCluster)?0:aParentClust))
        Error(_L("Start cluster of .. is not parent directory"),KErrNone);
    }



static  void TestParentDir(TBool aUseVfat)
    {

    test.Next(_L("TestParentDir()"));

    TInt root = gRootCluster;
    TInt cl   = gFirstDataCluster;
    TInt p1   = cl;

    FormatPack();

    TestMakeDir(_L("\\P1\\"), cl++, root);


    const TInt nDirEntries= gBytesPerCluster / KSizeOfFatDirEntry; //-- number of dir. entries to fill 1 cluster
    const TInt nFiles =  aUseVfat ? nDirEntries/2 : nDirEntries;   //-- number of 8.3 files to fill 1 cluster

    cl++;
    for (TInt i=0;i<nFiles;i++)
        {
        CreateFatEntry(_L("\\P1\\"), aUseVfat);
        }


    TInt p1p2 = cl;
    if(aUseVfat)
        {
        TestMakeDir(_L("\\p1\\p2\\"),       cl++, p1);
        TestMakeDir(_L("\\p1\\p21\\"),      cl++, p1);
        TestMakeDir(_L("\\p1\\p2\\p3\\"),   cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p33\\"),  cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p34\\"),  cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p35\\"),  cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p36\\"),  cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p37\\"),  cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p38\\"),  cl++, p1p2);
        }
    else
        {
        TestMakeDir(_L("\\P1\\P2\\"),       cl++, p1);
        TestMakeDir(_L("\\P1\\P21\\"),      cl++, p1);
        TestMakeDir(_L("\\P1\\P2\\P3\\"),   cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P33\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P34\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P35\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P36\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P37\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P38\\"),  cl++, p1p2);

        TestMakeDir(_L("\\P1\\P2\\P39\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P40\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P41\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P42\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P43\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P44\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P45\\"),  cl++, p1p2);
        }

    // if sectors/cluster == 1 then the directory \p1\p2\ will now have to
    // allocate another cluster
    if(TheBootSector.SectorsPerCluster()==1)
        ++cl;
    if(aUseVfat)
        {
        TestMakeDir(_L("\\p1\\p2\\p310\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p311\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p312\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p313\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p314\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p315\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p316\\"), cl++, p1p2);
        TestMakeDir(_L("\\p1\\p2\\p317\\"), cl++, p1p2);
        }
    else
        {
        TestMakeDir(_L("\\P1\\P2\\P310\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P311\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P312\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P313\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P314\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P315\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P316\\"),  cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P317\\"), cl++, p1p2);

        TestMakeDir(_L("\\P1\\P2\\P318\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P319\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P320\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P321\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P322\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P323\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P324\\"), cl++, p1p2);
        TestMakeDir(_L("\\P1\\P2\\P325\\"), cl++, p1p2);
        }

    // if sectors/cluster <= 2 then the directory \p1\p2\ will have to
    // allocate another cluster
    if(TheBootSector.SectorsPerCluster()<=2)
        ++cl;
    TestMakeDir(_L("\\P1\\P2\\P330\\"),  cl++, p1p2);
    TestMakeDir(_L("\\P11\\"),           cl++, root);
    }

static const TInt KMaxFiles=5;

//
// Test root dir size
//
static  void TestRoot()
    {
    test.Next(_L("Test root dir size"));

    if (gDiskType == EFat32)
        {
        test.Printf(_L("Not possible on FAT32 filesystem\n"));
        return;
        }

    FormatPack();
    TInt rootEntries=TheBootSector.RootDirEntries();
    test.Printf(_L("Total root entries allowed = %d\n"),rootEntries);
    TFileName fileName[KMaxFiles];  //  KMaxFiles=5 in this test
    TFileName tempName;
    TInt numberOfEntries=rootEntries;
    TInt r;
    RFile f;

    //-- generate 8.3 FAT entries, temp files created in upper-case, otherwise it will be 2 vFAT entries
    while(numberOfEntries--)
        {
        if (numberOfEntries<KMaxFiles)
            CreateFatEntry(_L("\\"), EFalse, &fileName[numberOfEntries]);
        else
            CreateFatEntry(_L("\\"), EFalse);

        }

    r = f.Create(TheFs, _L("\\123456.78"), EFileRead|EFileWrite);
    test_Value(r, r == KErrDirFull);
    f.Close();


    TInt i=0;
    for (i=0;i<KMaxFiles;i++)
        {
        r=TheFs.Delete(fileName[i]);
        test_KErrNone(r);
        }

    r=TheFs.SetSessionPath(_L("\\"));
    test_KErrNone(r);

    TInt nameLength=(KMaxFiles-1)*13;   // -1 for zero terminator
    CreateLongName(tempName,gSeed,nameLength*2);
    r=f.Create(TheFs,tempName,0);       //  Needs 9 free entries - there are only 5 available
    test_Value(r, r == KErrDirFull);
    tempName.SetLength(nameLength+1);
    r=f.Create(TheFs,tempName,0);       //  Needs 6 free entries - there are only 5 available
    test_Value(r, r == KErrDirFull);
    tempName.SetLength(nameLength);
    r=f.Create(TheFs,tempName,0);       //  Needs 5 free entries - there are 5 available
    test_KErrNone(r);
    f.Close();

#if 0       // This is the old test that assumed UNICODE builds
            // which created VFAT entries even for uppercase 8.3 file names
    TInt i=0;
    for (i=0;i<KMaxFiles-2;i++)
        {
        r=TheFs.Delete(fileName[i]);    //  UNICODE build - free 6 entries (delete 3 files)
        test_KErrNone(r);
        }

    r=TheFs.SetSessionPath(_L("\\"));
    test_KErrNone(r);

    TInt vFatUnitNameSize=13;
    TInt nameLength=(KMaxFiles-1)*vFatUnitNameSize-1;   //
    CreateLongName(tempName,gSeed,nameLength*2);
    r=f.Create(TheFs,tempName,0);                       //  Needs 9 free entries
    test_Value(r, r == KErrDirFull);

    nameLength=(KMaxFiles)*vFatUnitNameSize;
    tempName.SetLength(nameLength+1);
    r=f.Create(TheFs,tempName,0);                       //  Needs 7 free entries
    test_Value(r, r == KErrDirFull);
    tempName.SetLength(nameLength);
    r=f.Create(TheFs,tempName,0);                       //  Needs 6 free entries
    test_KErrNone(r);
    f.Close();
#endif

    TheFs.Delete(tempName);
    tempName.SetLength(nameLength-7);
    r=f.Create(TheFs,tempName,0);
    test_KErrNone(r);
    f.Close();

    r=f.Create(TheFs,_L("ASDF"),0);
    test_Value(r, r == KErrDirFull);

    TheFs.Delete(tempName);
    tempName.SetLength(nameLength-15);
    r=f.Create(TheFs,tempName,0);
    test_KErrNone(r);
    f.Close();

    tempName=_L("testname");
    r=f.Create(TheFs,tempName,0);
    test_Value(r, r == KErrDirFull);
    tempName.UpperCase();
    r=f.Create(TheFs,tempName,0);
    test_KErrNone(r);
    f.Close();


    r=TheFs.SetSessionPath(gSessionPath);
    test_KErrNone(r);
    }

static  void TestVolumeSize()
//
// Test the volume size is zero when empty
//
    {
    test.Next(_L("Test the volume size"));
    FormatPack();

    TVolumeInfo volInfo;
    TInt r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    TInt64 calcsize = MAKE_TINT64(0, gClusterCount)*gBytesPerCluster;
    if (volInfo.iSize > calcsize)
        {
        test.Printf(_L("volInfo.iSize = %ld\n"), volInfo.iSize);
        test.Printf(_L("volInfo.iFree = %ld\n"), volInfo.iFree);
        test.Printf(_L("calculated    = %ld\n"), calcsize);
        TInt diff = I64LOW(volInfo.iSize-calcsize);
        test.Printf(_L("difference    = %d (%d clusters)\n"), diff, diff/gBytesPerCluster);
        test(0);
        }
    if (gDiskType == EFat32)
        volInfo.iSize -= gBytesPerCluster; // root dir is part of the 'size'
    if (volInfo.iSize != volInfo.iFree)
        {
        test.Printf(_L("volInfo.iSize = %ld\n"), volInfo.iSize);
        test.Printf(_L("volInfo.iFree = %ld\n"), volInfo.iFree);
        TInt diff = I64LOW(volInfo.iSize-volInfo.iFree);
        test.Printf(_L("difference    = %d (%d clusters)\n"), diff, diff/gBytesPerCluster);
        DumpData();
        DumpFat();
        test(0);
        }

    RFile f[KMaxFiles];
    TFileName fileName;
    TInt i=0;
    for (i=0;i<KMaxFiles;i++)
        {
        fileName=_L("\\File");
        fileName.AppendNum(i);
        r=f[i].Create(TheFs,fileName,0);
        test_KErrNone(r);
        }

    TInt maxTotalSize=1048576;
    TInt maxFileSize=maxTotalSize/KMaxFiles;
    TInt maxIterations=20;

    while(maxIterations--)
        {
        for (i=0;i<KMaxFiles;i++)
            {
            TInt randSize=Math::Rand(gSeed)%maxFileSize;
            r=f[i].SetSize(randSize);
            test_KErrNone(r);
            }
        test.Printf(_L("Countdown .. %d   \r"),maxIterations);
        }

    test.Printf(_L("\n"));

    TInt totalSize=0;

    for (i=0;i<KMaxFiles;i++)
        {
        TInt size=0;
        r=f[i].Size(size);
        test_KErrNone(r);
        totalSize+=((size+gBytesPerCluster-1)/gBytesPerCluster)*gBytesPerCluster;
        }

    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    if (gDiskType == EFat32)
        volInfo.iSize -= gBytesPerCluster; // root dir is part of the 'size'
    if (volInfo.iSize-volInfo.iFree!=totalSize)
        {
        test.Printf(_L("volInfo.iSize = %ld\n"), volInfo.iSize);
        test.Printf(_L("volInfo.iFree = %ld\n"), volInfo.iFree);
        test.Printf(_L("totalSize     = %ld\n"), totalSize);
        TInt diff = I64LOW(volInfo.iSize-volInfo.iFree) - totalSize;
        test.Printf(_L("difference    = %d (%d clusters)\n"), diff, diff/gBytesPerCluster);
        }
    test(volInfo.iSize-volInfo.iFree==totalSize);

    for (i=0;i<KMaxFiles;i++)
        f[i].Close();

    for (i=0;i<KMaxFiles;i++)
        {
        fileName=_L("\\File");
        fileName.AppendNum(i);
        r=TheFs.Delete(fileName);
        test_KErrNone(r);
        }

    r=TheFs.Volume(volInfo);
    if (gDiskType == EFat32)
        volInfo.iSize -= gBytesPerCluster; // root dir is part of the 'size'
    test_KErrNone(r);
    test(volInfo.iSize-volInfo.iFree==0);

    MakeDir(gSessionPath);

    TInt entries=(gBytesPerCluster/KSizeOfFatDirEntry)*5-2;
    entries = ThrottleDirEntries(entries, 2);

    TInt clusters = ((entries * KSizeOfFatDirEntry) + gBytesPerCluster-1) / gBytesPerCluster;

    //-- create "entries" FAT dir. entries by creating 8.3 files in upper case
    while(entries--)
        {
        CreateFatEntry(gSessionPath, EFalse);
        }


    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    if (gDiskType == EFat32)
        volInfo.iSize -= gBytesPerCluster; // root dir is part of the 'size'
    test.Printf(_L("volInfo.iSize = %ld\n"), volInfo.iSize);
    test.Printf(_L("volInfo.iFree = %ld\n"), volInfo.iFree);
    if (volInfo.iSize-volInfo.iFree!=clusters*gBytesPerCluster)
        {
        DumpFat();
        DumpData(1, 200);
        }
    test(volInfo.iSize-volInfo.iFree==clusters*gBytesPerCluster);

    //-- create 1 FAT dir. entry
    CreateFatEntry(gSessionPath, EFalse);

    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    if (gDiskType == EFat32)
        volInfo.iSize -= gBytesPerCluster; // root dir is part of the 'size'
    test.Printf(_L("volInfo.iSize = %ld\n"), volInfo.iSize);
    test.Printf(_L("volInfo.iFree = %ld\n"), volInfo.iFree);
    if (volInfo.iSize-volInfo.iFree!=(clusters+1)*gBytesPerCluster)
        {
        DumpFat();
        DumpData(1, 200);
        }
    test(volInfo.iSize-volInfo.iFree==(clusters+1)*gBytesPerCluster);

    CFileMan* fMan=CFileMan::NewL(TheFs);
    r=fMan->RmDir(gSessionPath);
    test_KErrNone(r);
    delete fMan;
    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    if (gDiskType == EFat32)
        volInfo.iSize -= gBytesPerCluster; // root dir is part of the 'size'
    if (volInfo.iSize-volInfo.iFree!=0)
        {
        DumpFat();
        DumpData(1, 200);
        }
    test(volInfo.iSize-volInfo.iFree==0);
    }


//
// Writes a standard dos entry to the disk and checks that this can be read
// (in Unicode build)
//
static  void TestUnicodeEntry()
    {
    test.Next(_L("Test Unicode entry"));

    const TInt KDirEntrySize=32;

    FormatPack();
    DoReadBootSector(TheBootSector);
    TInt pos=gRootDirStart;

    TBuf8<KDirEntrySize> buffer;
    buffer.SetLength(KDirEntrySize);
    buffer.FillZ();
    buffer.Replace(0,11,_L8("TEST1      "));

    TInt r=TheDisk.Open(TheFs,CurrentDrive());
    test_KErrNone(r);
    r=TheDisk.Write(pos,buffer);
    test_KErrNone(r);
    TheDisk.Close();

    r=TheDir.Open(TheFs,_L("\\"),KEntryAttMaskSupported);
    test_KErrNone(r);
    r=TheDir.Read(TheEntry);
    test_KErrNone(r);
    test(TheEntry.iName==_L("TEST1"));
    r=TheDir.Read(TheEntry);
    test_Value(r, r == KErrEof);
    TheDir.Close();

    r=TheFs.SetSessionPath(_L("\\"));
    test_KErrNone(r);
    TEntry e;
    r=TheFs.Entry(_L("TEST1"),e);
    if(e.iName!=_L("TEST1"))
        {
        test.Printf(_L("e.iName = %S\n"),&e.iName);
        test(EFalse);
        }
    }

static  TUint32 GetValue(const TPtrC8& aData, TInt aOffset, TInt aLength)
    {
    TUint32 val = 0;
    while (aLength-- > 0)
        val = val * 256 + aData[aOffset+aLength];
    return val;
    }

static  void TestDiskIntegrity(TBool aTestOnly=EFalse)
//
// Does 'sanity checking' on the BPB and other areas
//
    {
    if (!aTestOnly)
        test.Next(_L("Test disk boot area integrity"));
    TInt seclen = TheBootSector.BytesPerSector();
    HBufC8 *bootp = HBufC8::NewL(seclen);
    TPtr8   boot((TUint8*)bootp, seclen);
    HBufC8 *backp = HBufC8::NewL(seclen);
    TPtr8   back((TUint8*)backp, seclen);
    HBufC8 *infop = HBufC8::NewL(seclen);
    TPtr8   info((TUint8*)bootp, seclen);
    TInt r=TheDisk.Open(TheFs,CurrentDrive());
    if (r != KErrNone)
        test.Printf(_L("Error %d opening on %C"), r, (TUint)gDriveToTest);
    test_KErrNone(r);
    r=TheDisk.Read(0, boot);
    test_KErrNone(r);
    TUint32 val = GetValue(boot, 510, 2);
    RDebug::Print(_L("BPB magic number = 0x%X\n"), val);
    test(aTestOnly || val == 0xAA55);
    switch (boot[0])
        {
        case 0xEB:
            RDebug::Print(_L("Jump %02X 0x%02X\n"), boot[0], boot[1]);
            test(aTestOnly || boot[2] == 0x90);
            break;
        case 0xE9:
            RDebug::Print(_L("Jump %02X 0x%02X%02X\n"), boot[0], boot[2], boot[1]);
            break;
        default:
            RDebug::Print(_L("Invalid boot start: %02X %02X %02X\n"), boot[0], boot[1], boot[2]);
            test(aTestOnly);
        }
    switch (gDiskType)
        {
        case EFat12:
            test(aTestOnly || TheBootSector.ReservedSectors() >= 1);
            test.Printf(_L("BPB sector OK\n"));
            break;
        case EFat16:
            test(aTestOnly || TheBootSector.ReservedSectors() >= 1);
            test.Printf(_L("BPB sector OK\n"));
            break;
        default:
            test(aTestOnly || TheBootSector.ReservedSectors() >= 1);
            test(aTestOnly || TheBootSector.ReservedSectors() > TheBootSector.BkBootRecSector());
            test(aTestOnly || TheBootSector.ReservedSectors() > TheBootSector.FSInfoSectorNum());
            test.Printf(_L("BPB sector OK\n"));
            if (TheBootSector.BkBootRecSector() > 0)
                {
                r=TheDisk.Read(TheBootSector.BkBootRecSector()*seclen, back);
                test(aTestOnly || r==KErrNone);
                if (boot != back)
                    {
                    RDebug::Print(_L("Boot sector != backup\n"));
                    RDebug::Print(_L("Sector 0: Boot sector\n"));
                    DumpHex(boot.Ptr(), seclen);
                    RDebug::Print(_L("Sector %d: Backup sector\n"), TheBootSector.BkBootRecSector());
                    DumpHex(back.Ptr(), seclen);
                    test(aTestOnly);
                    }
                test.Printf(_L("Backup BPB sector OK\n"));
                }
            else
                test.Printf(_L("Backup BPB not present\n"));
            if (TheBootSector.FSInfoSectorNum() > 0)
                {
                r=TheDisk.Read(TheBootSector.FSInfoSectorNum()*seclen, info);
                test(aTestOnly || r==KErrNone);
                // Test the 'magic numbers' (signatures) as specified
                val = GetValue(info, 0, 4);
                RDebug::Print(_L("FSI signature 1  = 0x%X\n"), val);
                test(aTestOnly || val == 0x41615252);
                val = GetValue(info, 484, 4);
                RDebug::Print(_L("FSI signature 2  = 0x%X\n"), val);
                test(aTestOnly || val == 0x61417272);
                val = GetValue(info, 508, 4);
                RDebug::Print(_L("FSI magic number = 0x%X\n"), val);
                test(aTestOnly || val == 0xAA550000);
                // Check the last known free count and the next free cluster value.  If
                // they are not calculated they should be 0xFFFFFFFF, otherwise must be
                // less than the number of clusters.
                val = GetValue(info, 488, 4);
                RDebug::Print(_L("FSI last free #  = 0x%X\n"), val);
                test(aTestOnly || val == 0xFFFFFFFF || val <= (TUint32)gClusterCount);
                val = GetValue(info, 492, 4);
                RDebug::Print(_L("FSI next free #  = 0x%X\n"), val);
                test(aTestOnly || val == 0xFFFFFFFF || val < (TUint32)gClusterCount);
                test.Printf(_L("FSInfo sector OK\n"));
                }
            break;
        }
    TheDisk.Close();
    delete bootp;
    delete backp;
    delete infop;
    }

static  void TestFATTableEntries()
//
// Test that reading/writing FAT table entries preserves the upper 4 bits of data.
//
    {
    test.Next(_L("Test reading/writing FAT table entries"));
    FormatPack();

    TUint32 buf[16];
    TInt i=0;
    TInt r=KErrNone;

    for (i=0; i <=7; i++)
        {
        buf[i] = GetFatEntry(i);
        }

    test.Printf(_L("First 8 FAT Entries before signature: \n"));
    test.Printf(_L("%08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x\n"),
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    for (i=0; i <=7; i++)
        {
        MarkFatEntry(i);
        }

    for (i=0; i <=7; i++)
        {
        buf[i] = GetFatEntry(i);
        }

    test.Printf(_L("First 8 FAT Entries after signature: \n"));
    test.Printf(_L("%08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x\n"),
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);


    test(TheFile.Create(TheFs,_L("\\CORRUPT1.TMP"),EFileRead|EFileWrite)==KErrNone);

    TheBuffer.SetLength(2048);
    Mem::Fill(&TheBuffer[0],2048,'X');

    for(i=0; i<=20; i++)
        {
        r = TheFile.Write(TheBuffer);
        test_KErrNone(r);
        }

    TheFile.Close();

    for (i=8; i <=15; i++)
        {
        buf[i] = GetFatEntry(i-8);
        }

    test.Printf(_L("First 8 FAT Entries after file write: \n"));
    test.Printf(_L("%08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x\n"),
                buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

    for (i=0; i<=7; i++)
        {
        test((buf[i] & 0xF0000000) == (buf[i+8] & 0xF0000000));
        }

    test.Printf(_L("Top 4 bits of first 8 FAT Entries have been preserved.\n"));
    }


//-----------------------------------------------------------------------------
/**
    Test that FAT[0] and FAT[1] just after formatting are compliant to FAT specs.
    So that this test step shall be called just after the volume formatted.
*/
static void TestFirst2FatEntries()
{
    test.Next(_L("Test FAT[0] and FAT[1] after formatting"));

    TInt nRes;
    TBuf8<8> fat1Buf; //-- buffer for FAT[0] & FAT[1] read from 1st FAT copy
    TBuf8<8> fatBufCurr;

    //-- read first several FAT entries from FAT1
    const TUint32 posFat1Start = TheBootSector.FirstFatSector() * TheBootSector.BytesPerSector();
    const TUint32 fatSize = TheBootSector.TotalFatSectors() * TheBootSector.BytesPerSector();
    const TInt numFATs = TheBootSector.NumberOfFats();


    nRes = MediaRawRead(TheFs, CurrentDrive(), posFat1Start, 8, fat1Buf);
    test(nRes==KErrNone);

    switch(gDiskType)
    {
     //----------- FAT12 ---------------------
     case EFat12:
     {
        fat1Buf.SetLength(3); //-- FAT12 entry occupies 1.5 bytes
        test.Printf(_L("FAT12, first 2 entries: %x %x %x\n"), fat1Buf[0], fat1Buf[1], fat1Buf[2]);

        test(fat1Buf[0]==0xF8 && fat1Buf[1]==0xFF && fat1Buf[2]==0xFF); //-- see FAT specs, these are first 2 entries

        //-- test that all copies of FAT have the same values in FAT[0] & FAT[1]
        for(TInt i=1; i<numFATs; ++i)
        {
            nRes = MediaRawRead(TheFs, CurrentDrive(), posFat1Start + i*fatSize, 8, fatBufCurr);
            test(nRes==KErrNone);

            fatBufCurr.SetLength(3);

            if(fatBufCurr != fat1Buf)
            {
                test.Printf(_L("1st 2 FAT entries in FAT#%d are different from FAT1!\n"), i);
                test(0);
            }
        }


     }
     break;

     //----------- FAT16 ---------------------
     case EFat16:
     {
        typedef TUint16 TFat16Entry;

        fat1Buf.SetLength(2*sizeof(TFat16Entry));
        const TFat16Entry* pFat = (const TFat16Entry*)fat1Buf.Ptr();

        const TFat16Entry fatEntry_0 = pFat[0]; //-- do not mask entries
        const TFat16Entry fatEntry_1 = pFat[1]; //-- do not mask entries

        test.Printf(_L("FAT16[0]=0x%x, FAT16[1]=0x%x\n"), fatEntry_0, fatEntry_1);

        test(fatEntry_0 == 0xFFF8); //-- see FAT specs
        test(fatEntry_1 == 0xFFFF); //-- the volume shall be clean just after the formatting. It can be 0x7FFF if a write to the volume occured.

        //-- test that all copies of FAT have the same values in FAT[0] & FAT[1]
        for(TInt i=1; i<numFATs; ++i)
        {
            nRes = MediaRawRead(TheFs, CurrentDrive(), posFat1Start + i*fatSize, 8, fatBufCurr);
            test(nRes==KErrNone);

            fatBufCurr.SetLength(2*sizeof(TFat16Entry));

            if(fatBufCurr != fat1Buf)
            {
                test.Printf(_L("1st 2 FAT entries in FAT#%d are different from FAT1!\n"), i);
                test(0);
            }
        }

     }
     break;

     //----------- FAT32 ---------------------
     case EFat32:
     {
        typedef TUint32 TFat32Entry;

        fat1Buf.SetLength(2*sizeof(TFat32Entry));
        const TFat32Entry* pFat = (const TFat32Entry*)fat1Buf.Ptr();

        const TFat32Entry fatEntry_0 = pFat[0]; //-- do not mask entries
        const TFat32Entry fatEntry_1 = pFat[1]; //-- do not mask entries

        test.Printf(_L("FAT32[0]=0x%x, FAT32[1]=0x%x\n"), fatEntry_0, fatEntry_1);

        test(fatEntry_0 == 0x0FFFFFF8); //-- see FAT specs
        test(fatEntry_1 == 0x0FFFFFFF); //-- the volume shall be clean just after the formatting. It can be 0x07FFFFFF if a write to the volume occured.

        //-- test that all copies of FAT have the same values in FAT[0] & FAT[1]
        for(TInt i=1; i<numFATs; ++i)
        {
            nRes = MediaRawRead(TheFs, CurrentDrive(), posFat1Start + i*fatSize, 8, fatBufCurr);
            test(nRes==KErrNone);

            fatBufCurr.SetLength(2*sizeof(TFat32Entry));

            if(fatBufCurr != fat1Buf)
            {
                test.Printf(_L("1st 2 FAT entries in FAT#%d are different from FAT1!\n"), i);
                test(0);
            }
        }
     }
     break;

     default:
        test(0);
     break;

    };//switch(gDiskType)



}


/**
Exhaustive test of Data alignmemnt calculation
in this code the function
    TInt TFatAlignment::AdjustFirstDataSectorAlignment(TInt aBlockSize)
should be exactly the same as
    TInt CFatFormatCB::AdjustFirstDataSectorAlignment(TInt aBlockSize)
*/
class TFatAlignment
    {
public:
    enum {KDefFatResvdSec = 1, KDefFat32ResvdSec = 32}; ///< default number of FAT32 reserved sectors
public:
    TFatAlignment();
    void Init(TBool aFat32, TInt aNumberOfFats, TInt aMaxDiskSectors, TInt aSectorsPerCluster, TInt aRootDirEntries);
    TUint32 MaxFat32Sectors() const;
    TInt MaxFat16Sectors() const;
    TInt MaxFat12Sectors() const;
    TUint32 RootDirSectors() const;
    TInt FirstDataSector() const;
    TBool Is32BitFat() const;
    TBool Is16BitFat() const;

    TInt AdjustFirstDataSectorAlignment(TInt aBlockSize);
    void Display();
public:
    TInt iBytesPerSector;
    TInt iNumberOfFats;
    TInt iMaxDiskSectors;
    TInt iSectorsPerCluster;
    TInt iReservedSectors;
    TInt iSectorsPerFat;
    TInt iRootDirEntries;

    TBool iFat32;   // 0 = FAT16, 1 = FAT32
    TInt iMaxIterations;
    };

TFatAlignment::TFatAlignment()
    {
    iMaxIterations = 0;
    }

void TFatAlignment::Init(TBool aFat32, TInt aNumberOfFats, TInt aMaxDiskSectors, TInt aSectorsPerCluster, TInt aRootDirEntries)
    {
    iBytesPerSector = 512;
    iFat32 = aFat32;
    iNumberOfFats = aNumberOfFats;
    iMaxDiskSectors = aMaxDiskSectors;
    iSectorsPerCluster = aSectorsPerCluster;
    iRootDirEntries = aRootDirEntries;

    iReservedSectors = iFat32 ? KDefFat32ResvdSec : KDefFatResvdSec;
    iSectorsPerFat = iFat32 ? MaxFat32Sectors() : MaxFat16Sectors();
    }

void TFatAlignment::Display()
    {
    RDebug::Print(_L("iFat32 %u iNumberOfFats %u,iMaxDiskSectors %u,iSectorsPerCluster %u,iReservedSectors %u,iSectorsPerFat %u, iRootDirEntries %u, FirstDataSector %08X"),
        iFat32,
        iNumberOfFats,
        iMaxDiskSectors,
        iSectorsPerCluster,
        iReservedSectors,
        iSectorsPerFat,
        iRootDirEntries,
        FirstDataSector());
    }

TInt TFatAlignment::MaxFat16Sectors() const
    {

    TInt fatSizeInBytes=(2*iMaxDiskSectors)/iSectorsPerCluster+(iBytesPerSector-1);
    return(fatSizeInBytes/iBytesPerSector);
    }


TInt TFatAlignment::MaxFat12Sectors() const
    {
    TInt maxDiskClusters=iMaxDiskSectors/iSectorsPerCluster;
    TInt fatSizeInBytes=maxDiskClusters+(maxDiskClusters>>1)+(iBytesPerSector-1);
    return(fatSizeInBytes/iBytesPerSector);
    }


TUint32 TFatAlignment::MaxFat32Sectors() const
    {
    TUint32 calc1 = iMaxDiskSectors - iReservedSectors;
    TUint32 calc2 = (256 * iSectorsPerCluster) + iNumberOfFats;
    calc2 = calc2 >> 1;
    return (calc1 + (calc2 - 1))/calc2;
    }


/**
    @return Number of sectors in root directory. 0 for FAT32
*/
TUint32 TFatAlignment::RootDirSectors() const
    {
    const TInt KSizeOfFatDirEntry       =32;    ///< Size in bytes of a Fat directry entry

    return ( (iRootDirEntries * KSizeOfFatDirEntry + (iBytesPerSector-1)) / iBytesPerSector );
    }

TInt TFatAlignment::FirstDataSector() const
    {
    return( iReservedSectors + iNumberOfFats * iSectorsPerFat + RootDirSectors());
    }

TBool TFatAlignment::Is32BitFat() const
    {
    return iFat32;
    }

TBool TFatAlignment::Is16BitFat() const
    {
    return !iFat32;
    }

#define __PRINT1


// AdjustFirstDataSectorAlignment()
// Attempts to align the first data sector on an erase block boundary by modifying the
// number of reserved sectors.
TInt TFatAlignment::AdjustFirstDataSectorAlignment(TInt aEraseBlockSizeInSectors)
    {
    const TBool bFat16 = Is16BitFat();
    const TBool bFat32 = Is32BitFat();

    // Save these 2 values in the event of a convergence failure; this should
    // hopefully never happen, but we will cater for this in release mode to be safe,
    TInt reservedSectorsSaved = iReservedSectors;
    TInt sectorsPerFatSaved = iSectorsPerFat;

    TInt reservedSectorsOld = 0;

    // zero for FAT32
    TInt rootDirSectors = (iRootDirEntries * KSizeOfFatDirEntry + (iBytesPerSector-1)) / iBytesPerSector;
    TInt fatSectors = 0;

    TInt KMaxIterations = 10;
    TInt n;
    for (n=0; n<KMaxIterations && reservedSectorsOld != iReservedSectors; n++)
        {
        reservedSectorsOld = iReservedSectors;

        iSectorsPerFat = bFat32 ? MaxFat32Sectors() : bFat16 ? MaxFat16Sectors() : MaxFat12Sectors();

        fatSectors = iSectorsPerFat * iNumberOfFats;

        // calculate number of blocks
        TInt  nBlocks = (iReservedSectors + fatSectors + rootDirSectors + aEraseBlockSizeInSectors-1) / aEraseBlockSizeInSectors;

        iReservedSectors = (nBlocks * aEraseBlockSizeInSectors) - rootDirSectors - fatSectors;
        }

    ASSERT(iReservedSectors >= (TInt) (bFat32 ? KDefFat32ResvdSec : KDefFatResvdSec));

    if ((FirstDataSector() & (aEraseBlockSizeInSectors-1)) == 0)
        {
        return KErrNone;
        }
    else
        {
        iReservedSectors = reservedSectorsSaved;
        iSectorsPerFat = sectorsPerFatSaved;
        return KErrGeneral;
        }
    }


void TestFirstDataSectorAlignment()
    {
    test.Start(_L("Exhaustive test of data alignment calculation"));

    typedef struct
        {
        TInt iNumberOfFats;
        TInt iMaxDiskSectors;
        TInt iSectorsPerCluster;
        TInt iBlockSize;
        TInt iRootDirEntries;
        } STestVal;
    STestVal testVals[] =
        {
            {2, 15720448, 32, 16*1024, 0},  // 4GB MoviNand, cluster size = 16K
            {2, 106496, 2, 2048, 512},  // diskSize = 54MB,  = block size = 1MB
            {2, 1048576, 8, 2048, 0},   // diskSize = 512 MB
            {2, 1048578, 8, 2048, 0},   // Doesn't converge with original algorithm
        };

    TFatAlignment fatAlignment;
    TInt numOfTests = sizeof(testVals) / sizeof(STestVal);
    for (TInt n=0; n<numOfTests; n++)
        {
        STestVal& testVal = testVals[n];
        TBool fat32 = testVal.iMaxDiskSectors >= 1048576;

        fatAlignment.Init(
            fat32,
            testVal.iNumberOfFats,
            testVal.iMaxDiskSectors,
            testVal.iSectorsPerCluster,
            testVal.iRootDirEntries);
        TInt r = fatAlignment.AdjustFirstDataSectorAlignment(testVal.iBlockSize);
        test_KErrNone(r);
        fatAlignment.Display();
        }

    const TInt64 KOneMByte = 1024*1024;
    const TInt64 KOneGByte = 1024*KOneMByte;
    const TInt64 KLastSizeToTest = 32*KOneGByte;
    TInt iteration=0;
    TInt64 diskSize;



    TInt successes = 0;
    TInt failures = 0;

    for (iteration=0, diskSize = 16*KOneMByte; diskSize < KLastSizeToTest; iteration++, diskSize+=512)
        {
        TInt diskSizeInSectors = (TInt) (diskSize >> 9);

        const TInt KMaxFAT16Entries=0xFFF0;     ///< Maximum number of clusters in a Fat16 Fat table, 65520

        TBool fat32 = EFalse;
        TInt numberOfFats = 2;
        TInt rootDirEntries;
        TInt sectorsPerCluster;
        TInt blockSizeInSectors = 32;   // 16K for FAT16

        if (diskSizeInSectors<4096) // < 2MB
            {
            rootDirEntries=128;
            sectorsPerCluster=1;
            }
        else if (diskSizeInSectors<8400) // < 4MB
            {
            rootDirEntries=256;
            sectorsPerCluster=2;
            }
        else if (diskSizeInSectors<16384) // < 8MB
            {
            rootDirEntries=512;
            sectorsPerCluster=4;
            }
        else if (diskSizeInSectors<32680) // < 16MB
            {
            rootDirEntries=512;
            sectorsPerCluster=8;
            }
        else if(diskSizeInSectors<1048576) // >= 16Mb - FAT16   < (1048576) 512MB
            {
            TInt minSectorsPerCluster=(diskSizeInSectors+KMaxFAT16Entries-1)/KMaxFAT16Entries;
            rootDirEntries=512;
            sectorsPerCluster=1;
            while (minSectorsPerCluster>sectorsPerCluster)
                sectorsPerCluster<<=1;
            }
        else    //use FAT32
            {
            rootDirEntries=0;                       //this is always the case for fat32
            if(diskSizeInSectors < 16777216)        //8GB in 512byte sectors
                sectorsPerCluster=8;
            else if(diskSizeInSectors < 33554432)   //16GB in 512byte sectors
                sectorsPerCluster=16;
            else if(diskSizeInSectors < 67108864)   //32GB in 512byte sectors
                sectorsPerCluster=32;
            else
                sectorsPerCluster=64;               //Anything >= 32GB uses a 32K cluster size
            blockSizeInSectors = 2048;          // 1MB for FAT32
            fat32 = ETrue;
            }


        fatAlignment.Init(
            fat32,
            numberOfFats,
            diskSizeInSectors,
            sectorsPerCluster,
            rootDirEntries);
        TInt r = fatAlignment.AdjustFirstDataSectorAlignment(blockSizeInSectors);
        if (r == KErrNone)
            successes++;
        else
            failures++;


//      if (diskSize % 0x08000000 == 0)
//          {
//          RDebug::Print(_L("Iter %10lX of %10lX"), diskSize, KLastSizeToTest);
//          fatAlignment.Display();
//          }
        }
    RDebug::Print(_L("Total iterations %u"), iteration);
    RDebug::Print(_L("Max loop count %u"), fatAlignment.iMaxIterations);
    RDebug::Print(_L("successes %d failures %d, success rate %ld"),
        successes, failures, (TInt64(successes) * 100) / TInt64(successes + failures));
    test (failures == 0);

    }


static void TestZeroLengthFile()
//
// Test what happens if you write more to a zero length file than
// will fit in the filesystem.
//
    {
    test.Next(_L("Test behaviour of extending a zero length file"));

    FormatPack();

    TInt r;

    TVolumeInfo volInfo;
    r=TheFs.Volume(volInfo);
    test_KErrNone(r);

    TInt64 spaceToUse = volInfo.iFree - gBytesPerCluster; // whole disk except 1 cluster

    test.Printf(_L("spaceToUse %ld gClusterCount %d gBytesPerCluster %d\n"), spaceToUse, gClusterCount, gBytesPerCluster);
    test.Printf(_L("Before fill, volInfo.iSize %ld volInfo.iFree %ld\n"), volInfo.iSize, volInfo.iFree);

    RFile f;

    TInt tempfiles = 0;
    while (spaceToUse > K1GigaByte)
        {
        TFileName tempName;
        r=f.Temp(TheFs,_L("\\"),tempName,EFileRead|EFileWrite);
        test_KErrNone(r);
        r=f.SetSize(K1GigaByte);
        test_KErrNone(r);
        f.Close();
        spaceToUse -= K1GigaByte;
        tempfiles++;
        }

    r=f.Replace(TheFs,_L("\\USESPACE.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    r=f.SetSize((TInt)spaceToUse);
    test_KErrNone(r);
    f.Close();

    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    test.Printf(_L("After fill, volInfo.iSize %ld volInfo.iFree %ld\n"), volInfo.iSize, volInfo.iFree);

    test(volInfo.iFree==gBytesPerCluster); // check we have 1 cluster free

    r=f.Replace(TheFs,_L("\\FILE.TMP"),EFileRead|EFileWrite);
    test_KErrNone(r);
    r=f.SetSize(2*gBytesPerCluster); // 2 clusters (will fail since there's not space)
    test_Value(r, r == KErrDiskFull);
    f.Close();

    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    test(volInfo.iFree==gBytesPerCluster); // check we still have 1 cluster free

    r=f.Replace(TheFs,_L("\\USESPACE.TMP"),EFileRead|EFileWrite); // truncate file to 0
    test_KErrNone(r);
    f.Close();

    r=TheFs.Volume(volInfo);
    test_KErrNone(r);
    test(volInfo.iFree==(spaceToUse+gBytesPerCluster)); // check we've freed up the space from USESPACE plus one cluster

    
    test(TheBootSector.IsValid()); //-- TheBootSector is read after formatting
    TInt64 rootDirpos = gRootDirStart;

    
    //-- read 1 sector of the root dir.
    r = MediaRawRead(TheFs, CurrentDrive(), rootDirpos, TheBootSector.BytesPerSector(), TheBuffer);
    test_KErrNone(r);

    const TFatDirEntry* pE=(TFatDirEntry*)TheBuffer.Ptr();
    while (tempfiles-- > 0)
        {
        while (pE->IsVFatEntry())
            pE++;
        test(pE->Size()==(TUint)K1GigaByte);
        pE++;
        }

    while (pE->IsVFatEntry())
        pE++;

    TBuf8<15> name=pE->Name();
    test(name==_L8("USESPACETMP"));
    test(pE->StartCluster()==0);

    pE++;
    while (pE->IsVFatEntry())
        pE++;

    name=pE->Name();
    test(name==_L8("FILE    TMP"));
    test(pE->StartCluster()==0);

    FormatPack();

    }


//
// Call tests that may leave
//
void CallTestsL()
    {

    //-- init random generator
    rndSeed = Math::Random();

    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());



    TInt drvNum;
    TInt r=TheFs.CharToDrive(gDriveToTest,drvNum);
    test_KErrNone(r);

    if (!Is_Fat(TheFs,drvNum))
        {
        test.Printf(_L("CallTestsL: Skipped: test requires FAT filesystem\n"));
        return;
        }


    //-- print drive information
    PrintDrvInfo(TheFs, drvNum);

    // check this is not the internal ram drive
    TVolumeInfo v;
    r=TheFs.Volume(v, drvNum);
    test_KErrNone(r);
    TBool isRamDrive = v.iDrive.iMediaAtt&KMediaAttVariableSize;

    gSessionPath[0] = (TText)gDriveToTest;
     // verify that the drive is large enough for proper testing
    if (v.iSize<512*1024)
        {
        test.Printf(_L("CallTestsL: Skipped: test not supported on drives smaller than 512 KB\n"));
        return;
        }

    FormatPack();
    DumpBootSector();

    test.Printf(_L("TotalSectors = %u (%u bytes)\n"),gTotalSectors,gTotalSectors*TheBootSector.BytesPerSector());
    test.Printf(_L("Sector size  = %u\n"),TheBootSector.BytesPerSector());
    test.Printf(_L("Cluster size = %u sectors\n"),TheBootSector.SectorsPerCluster());
    test.Printf(_L("Alloc unit   = %u\n"), gBytesPerCluster);
    test.Printf(_L("Fat is %u bit\n"), gFatBits);
    User::After(200000); // 1/5 secs

    // set up buffers
    TInt bufLen = 16*gBytesPerCluster;
    if (bufLen < 16*1024)
        bufLen = 16*1024;
    pBuffer1=HBufC8::NewL(bufLen);
    pBuffer2=HBufC8::NewL(bufLen);

    if (pBuffer1==NULL || pBuffer2==NULL)
        Error(_L("OOM"),KErrNoMemory);


    pBuffer1->Des().Zero();
    pBuffer1->Des().SetLength(bufLen);

    pBuffer2->Des().Zero();
    pBuffer2->Des().SetLength(bufLen);

    if (isRamDrive)
        {
        User::After(200000); // 1/5 secs
        test.Printf(_L("*** Tests not valid on internal ram drive %C:\n"), (TUint)gDriveToTest);
        User::After(200000); // 1/5 secs
        }
    else
        {
        TestZeroLengthFile();

#if defined(__WINS__)
        TestFirstDataSectorAlignment();
#endif

        TestFirst2FatEntries();

        TestDiskIntegrity();

        TestBounds();
        TestUnicodeEntry();

        TestClusters();

        TestClusterAllocation();


        TestParentDir(EFalse);  // Test without VFAT entries
        TestParentDir(ETrue);   // Test with VFAT entries

        TestRoot();
        TestVolumeSize();
        TestFATTableEntries();

        FormatPack();

        }
    delete pBuffer1;
    delete pBuffer2;
    }


/**
    Generate unique temp file name in upper (FAT entry) or lower case (2 VFAT entries)
    @param  aFN         descriptor for the file name
    @param  aUpperCase  if ETrue, the file name will be in upper case, in a lower case otherwise.

*/
void GenerateTmpFileName(TDes& aFN, TBool aUpperCase)
{
    const TInt rnd = Math::Rand(rndSeed);

    aFN.Format(_L("%08x.tmp"), rnd);

    if(aUpperCase)
        aFN.UpperCase();
    else
        aFN.LowerCase();

}

/**
    Create FAT or VFAT entry in a speciified directory

    @param  aDir        specifies the directory where enntry will be created
    @param  aVFatEntry  if true, VFAT entry will be created (2 FAT entries, actually), otherwise - FAT entry
    @param  apFileName  in !=NULL there will be placed the name of the file created
*/
void CreateFatEntry(const TDesC& aDir, TBool aVFatEntry, TDes *apFileName/*=NULL*/)
{
    TFileName tmpFN;
    RFile     file;
    TInt      nRes;

    do
    {
        GenerateTmpFileName(tmpFN, !aVFatEntry); //-- generates 8.3 file name FAT (1 entry) or VFAT (2 entries)
        tmpFN.Insert(0, aDir);

        nRes = file.Create(TheFs, tmpFN, EFileRead|EFileWrite);

        if(nRes == KErrAlreadyExists)
            continue; //-- current random name generator isn't perfect...

        if(nRes != KErrNone)
            Error(_L("Error creating a file"),nRes);

        file.Close();

    }while(nRes != KErrNone);

    if(apFileName)
        *apFileName = tmpFN;

}


