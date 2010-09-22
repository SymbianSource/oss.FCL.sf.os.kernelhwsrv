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
// f32test\filesystem\fat\t_scn32dr1.cpp
// Tests that ScanDrive fixes known errors to a Rugged FAT drive
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>

#include "t_server.h"

#include "fat_utils.h"
using namespace Fat_Test_Utils;

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

/*
T_testscndrv tests the scandrive utility. Errors in this test will be
introduced using the RRawdDisk class.  The correct fixup is tested by rereading
the disk.  Drives tested are the default path(epoc) and X: (wins). This test
returns immediately if used on the internal ram drive
*/

/*
  The initial FAT12 directory structure (with cluster number in brackets) is as follows:

  |
   - scndrv (2)
        |
        - dir1 (3-4)
        |   |
        |   <a very long file name (19 entries)> (5)
        |
        - dir2 (6)
            |
            - full (7)
            |   |
            |   |
            |   - <seven 2*32 bytes entries> (11-17)
            |
            - somedirwith3entries (8)
            |
            - somedir2with3entries (9)
            |
            - almostfull(10)
                |
                - <two lots of 6*32 bytes entries> (18+19)

*/

/*
  The initial FAT32 directory structure (with cluster number in brackets is as follows):

  |
   - scndrv (3)
        |
        - dir1 (4)
        |   |
        |   <a very long file name (19 entries)> (5)
        |
        - dir2 (6)
            |
            - full (7)
            |   |
            |   |
            |   - <seven 2*32 bytes entries> (11-17)
            |
            - somedirwith3entries (8)
            |
            - somedir2with3entries (9)
            |
            - almostfull(10)
                |
                - <two lots of 6*32 bytes entries> (18+19)

*/

GLDEF_D RTest test(_L("T_SCN32DR1"));

LOCAL_D const TInt KMaxFatEntries  = 2048;
LOCAL_D const TInt KMaxFatSize     = KMaxFatEntries * 4;

LOCAL_D const TInt KDirAttrReadOnly  = 0x01;
LOCAL_D const TInt KDirAttrHidden    = 0x02;
LOCAL_D const TInt KDirAttrSystem    = 0x04;
LOCAL_D const TInt KDirAttrVolumeId  = 0x08;
LOCAL_D const TInt KDirAttrDirectory = 0x10;
LOCAL_D const TInt KDirAttrArchive   = 0x20;
LOCAL_D const TInt KDirAttrLongName  = KDirAttrReadOnly | KDirAttrHidden | KDirAttrSystem | KDirAttrVolumeId;
LOCAL_D const TInt KDirAttrLongMask  = KDirAttrLongName | KDirAttrDirectory | KDirAttrArchive;
LOCAL_D const TInt KDirLastLongEntry = 0x40;

LOCAL_D RRawDisk TheRawDisk;
LOCAL_D TFatBootSector BootSector;
LOCAL_D TFileName TheDrive=_L("?:\\");
LOCAL_D HBufC8* FatBufPtr = NULL;
LOCAL_D HBufC8* DirBufPtr = NULL;
LOCAL_D HBufC8* ExtBufPtr = NULL;
LOCAL_D TInt32  ExtBufAdd = 0;
LOCAL_D TInt32  ExtBufLen = 0;
LOCAL_D HBufC8* FatDiskPtr = NULL;
LOCAL_D HBufC8* DirDiskPtr = NULL;

static TFatType gDiskType = EInvalid;

LOCAL_D TInt gDriveNumber;

LOCAL_D TInt gBytesPerCluster;
LOCAL_D TInt gEntriesPerCluster;
LOCAL_D TInt gRootDirSectors;
LOCAL_D TInt gRootDirEntries;
LOCAL_D TInt gRootDirStart;      // in bytes
LOCAL_D TInt gRootSector;
LOCAL_D TInt gFatStartBytes;
LOCAL_D TInt gFatTestSize;       // in bytes
LOCAL_D TInt gFatTestEntries;
LOCAL_D TInt gFatSizeSectors;
LOCAL_D TInt gFirstDataSector;
LOCAL_D TInt gMaxDataCluster;
LOCAL_D TInt gDataStartBytes;
LOCAL_D TInt gEndOfChain;        // for FAT12/16/32

// cluster numbers in 1 and >1 sector per cluster modes
LOCAL_D TInt gClusterRootDir;        //  2    2
LOCAL_D TInt gClusterScnDrv;         //  3    3
LOCAL_D TInt gClusterDir1;           //  4    4
LOCAL_D TInt gClusterDir1ext;        //  5    4
LOCAL_D TInt gClusterDir2;           //  7    6
LOCAL_D TInt gClusterDir2_Full;      //  8    7
LOCAL_D TInt gClusterDir2_SD3E;      //  9    8
LOCAL_D TInt gClusterDir2_SD23E;     // 10    9
LOCAL_D TInt gClusterDir2_AFull;     // 11   10
LOCAL_D TInt gClusterEndMaxDepth;    // 147   146

LOCAL_D TFileName LastInFull;

class TEntryInfo
    {
public:
    TEntryInfo(TInt aBytePos,TInt aLength):iBytePos(aBytePos),iLength(aLength){}
    TEntryInfo(){}
public:
    TInt iBytePos;
    TInt iLength;
    };


LOCAL_C TInt DirBufferSize()
//
// returns size in bytes nec for buffer to store relevant disk data
//
    {
    return(gMaxDataCluster*gBytesPerCluster);
    }

LOCAL_C TInt PosInBytes(TInt aFatIndex)
//
// Return number of bytes into the FAT
//
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

LOCAL_C TUint32 MaxClusters()
//
// Return the number of data clusters on the disk
//
    {
    TUint32 totSec = (BootSector.TotalSectors() ? BootSector.TotalSectors() : BootSector.HugeSectors());
    TUint32 numSec = totSec - gFirstDataSector;
    return numSec / BootSector.SectorsPerCluster();
    }

LOCAL_C TInt ClusterToByte(TInt aCluster)
//
// converts cluster number to byte offset on disk
//
    {
    TInt pos;
    if (aCluster < 2)
        pos = gRootDirStart;
    else
        pos = (aCluster - 2) * gBytesPerCluster + gFirstDataSector * BootSector.BytesPerSector();
    return pos;
    }

LOCAL_C TInt ByteToCluster(TInt aBytePos)
//
// Converts byte offset from root dir buffer to cluster number
//
    {
    if (aBytePos < gRootDirStart)
        return -1;
    if (aBytePos < gDataStartBytes)
        return 0;
    return (aBytePos - gDataStartBytes) / gBytesPerCluster + 2;
    }

LOCAL_C TInt ClusterEntryToBytes(TInt aCluster,TInt aEntry)
//
// converts position in cluster and entry number to byte pos from root directory
//
    {
    TInt pos;
    pos = ClusterToByte(aCluster) - gRootDirStart + aEntry*KSizeOfFatDirEntry;
    return pos;
    }

LOCAL_C TInt FindUnMatch(const TUint8* aBuf, const TUint8* aCmp, TInt aLen, TInt aStart=0)
//
// Return position in buffers which doesn't match, or -1 if it matches
//
    {
    for (TInt i = aStart; i < aStart + aLen; i++)
        if (aBuf[i] != aCmp[i])
            return i;
    return -1;
    }

LOCAL_C TUint32 GetFatEntry(TUint32 aIndex, const TUint8* aFat=NULL)
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
        pos += BootSector.ReservedSectors() * BootSector.BytesPerSector();
        TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
        test_KErrNone(r);
        TPtr8 buf(&data[0], 4);
        r=TheRawDisk.Read(pos, buf);
        test_KErrNone(r);
        TheRawDisk.Close();
        }

    TUint32 val = 0;
    switch (gDiskType)
        {
        case EFat32:
            val = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
            break;
        case EFat16:
            val = ptr[0] + (ptr[1] << 8);
            break;
        case EFat12:
            val = ptr[0] + (ptr[1] << 8);
            if (aIndex & 1)
                    val >>= 4;
            val &= 0xFFF;
            break;
        default:
            test(0);
        }
    return val;
    }

LOCAL_C void WriteToFatBuf(TInt aFatIndex,TInt aValue,const TUint8* aFat)
//
// Write a value to both fats starting at aFat
//
    {
    TUint8* p=(TUint8*)(aFat+PosInBytes(aFatIndex));
    switch (gDiskType)
        {
        case EFat32:
            p[0] = (TUint8) (aValue);
            p[1] = (TUint8) (aValue >> 8);
            p[2] = (TUint8) (aValue >> 16);
            p[3] = (TUint8) (aValue >> 24);
            break;
        case EFat16:
            p[0] = (TUint8) (aValue);
            p[1] = (TUint8) (aValue >> 8);
            break;
        case EFat12:
            {
            TUint8 mask=0x0F;
            TBool odd=(aFatIndex)&1;
            TUint8 fatVal;
            TInt value=aValue;
            if(odd)
                {
                mask<<=4;
                value<<=4;
                fatVal=p[0];
                fatVal&=~mask;
                fatVal|=(TUint8)(value&0xFF);
                p[0]=fatVal;
                p[1]=(TUint8)(value>>8);
                }
            else
                {
                p[0]=(TUint8)(value&0xFF);
                fatVal=p[1];
                fatVal&=~mask;
                fatVal|=(TUint8)(value>>8);
                p[1]=fatVal;
                }
            }
            break;
        default:
            test(0);
        }
    return;
    }

static void DoReadBootSector()
    {

    TInt nRes = ReadBootSector(TheFs, CurrentDrive(), KBootSectorNum<<KDefaultSectorLog2, BootSector);
    test_KErrNone(nRes);

    if(!BootSector.IsValid())
        {
        test.Printf(_L("Wrong bootsector! Dump:\n"));
        BootSector.PrintDebugInfo();
        test(0);
        }

    // Calculate derived variables (fixed for a particular disk format)
    if (BootSector.RootDirEntries() == 0)
        {
        test.Printf(_L("Disk is FAT32\n"));
        gDiskType = EFat32;
        gEndOfChain = 0x0FFFFFFF;
        }
    else if (BootSector.FatType() == EFat16)
        {
        test.Printf(_L("Disk is FAT16\n"));
        gDiskType = EFat16;
        gEndOfChain = 0xFFFF;
        }
    else
        {
        test.Printf(_L("Disk is FAT12\n"));
        gDiskType = EFat12;
        gEndOfChain = 0x0FFF;
        }

    gBytesPerCluster   = BootSector.BytesPerSector() * BootSector.SectorsPerCluster();
    gFatStartBytes     = BootSector.ReservedSectors() * BootSector.BytesPerSector();
    gEntriesPerCluster = gBytesPerCluster / KSizeOfFatDirEntry;

    TBool big = (BootSector.SectorsPerCluster() > 1);
    switch (gDiskType)
        {
        case EFat12:
        case EFat16:
            gRootDirEntries     = BootSector.RootDirEntries();
            gRootDirSectors     = ((gRootDirEntries * KSizeOfFatDirEntry + BootSector.BytesPerSector() - 1) / BootSector.BytesPerSector());
            gFatSizeSectors     = BootSector.FatSectors();
            gRootSector         = BootSector.ReservedSectors() + BootSector.NumberOfFats() * gFatSizeSectors;
            gFirstDataSector    = gRootSector + gRootDirSectors;
            gDataStartBytes     = gFirstDataSector * BootSector.BytesPerSector();
            gRootDirStart       = gRootSector * BootSector.BytesPerSector();
            gClusterRootDir     = (big ?   0 :   0);
            gClusterScnDrv      = (big ?   2 :   2);
            gClusterDir1        = (big ?   3 :   3);
            gClusterDir1ext     = (big ?   3 :   4);
            gClusterDir2        = (big ?   5 :   6);
            gClusterDir2_Full   = (big ?   6 :   7);
            gClusterDir2_SD3E   = (big ?   7 :   8);
            gClusterDir2_SD23E  = (big ?   8 :   9);
            gClusterDir2_AFull  = (big ?   9 :  10);
            gClusterEndMaxDepth = (big ? 145 : 146);
            break;
        case EFat32:
            //
            // FAT32 will alway pre-allocate a single cluster for the root directory
            //
            //  - The following calculations may look wierd (as the spec says that the FAT32 root dir
            //    is not fixed) but for the purposes of this test we assume that root dir is only
            //    one cluster in size, so we don't fill up the disk trying to fill up the root dir.
            //
            gRootDirEntries     = gBytesPerCluster * 1 / KSizeOfFatDirEntry;    // Maximum entries within default FAT32 root directory (before extension)
            gRootDirSectors     = 0;                                            // FAT32 has no fixed root directory sectors over which to skip
            gFatSizeSectors     = BootSector.FatSectors32();

            gRootSector         = BootSector.ReservedSectors() + BootSector.NumberOfFats() * gFatSizeSectors;
            gFirstDataSector    = gRootSector;

            gDataStartBytes     = gFirstDataSector * BootSector.BytesPerSector();
            gRootDirStart       = (BootSector.RootClusterNum() - 2) * gBytesPerCluster + gDataStartBytes;

            gClusterRootDir     = (big ?   2 :   2);
            gClusterScnDrv      = (big ?   3 :   3);
            gClusterDir1        = (big ?   4 :   4);
            gClusterDir1ext     = (big ?   4 :   5);
            gClusterDir2        = (big ?   6 :   7);
            gClusterDir2_Full   = (big ?   7 :   8);
            gClusterDir2_SD3E   = (big ?   8 :   9);
            gClusterDir2_SD23E  = (big ?   9 :  10);
            gClusterDir2_AFull  = (big ?  10 :  11);
            gClusterEndMaxDepth = (big ? 146 : 147);
            break;
        default:
            break;
        }

    gMaxDataCluster = gClusterDir2_AFull + 2 + (gFirstDataSector / BootSector.SectorsPerCluster() + 1);

    gFatTestEntries = MaxClusters();
    if (gFatTestEntries > KMaxFatSize)
        gFatTestEntries = KMaxFatSize;
    }


GLDEF_C void DumpBootSector()
//
// Display (in log) TFatBootSector structure
//
    {
    RDebug::Print(_L("iBytesPerSector    = %8d"), BootSector.BytesPerSector());
    RDebug::Print(_L("iSectorsPerCluster = %8d"), BootSector.SectorsPerCluster());
    RDebug::Print(_L("iReservedSectors   = %8d"), BootSector.ReservedSectors());
    RDebug::Print(_L("iNumberOfFats      = %8d"), BootSector.NumberOfFats());
    RDebug::Print(_L("iRootDirEntries    = %8d"), BootSector.RootDirEntries());
    RDebug::Print(_L("iTotalSectors      = %8d"), BootSector.TotalSectors());
    RDebug::Print(_L("iMediaDescriptor   = %8d"), BootSector.MediaDescriptor());
    RDebug::Print(_L("iFatSectors        = %8d"), BootSector.FatSectors());
    RDebug::Print(_L("iSectorsPerTrack   = %8d"), BootSector.SectorsPerTrack());
    RDebug::Print(_L("iNumberOfHeads     = %8d"), BootSector.NumberOfHeads());
    RDebug::Print(_L("iHiddenSectors     = %8d"), BootSector.HiddenSectors());
    RDebug::Print(_L("iHugeSectors       = %8d"), BootSector.HugeSectors());

    if (gDiskType == EFat32)
        {
        RDebug::Print(_L("iFatSectors32      = %8d"), BootSector.FatSectors32());
        RDebug::Print(_L("iFATFlags          = %8d"), BootSector.FATFlags());
        RDebug::Print(_L("iVersionNumber     = %8d"), BootSector.VersionNumber());
        RDebug::Print(_L("iRootClusterNum    = %8d (0x%08X)"), BootSector.RootClusterNum(), gRootDirStart);
        RDebug::Print(_L("iFSInfoSectorNum   = %8d (0x%08X)"), BootSector.FSInfoSectorNum(), BootSector.FSInfoSectorNum() * BootSector.BytesPerSector());
        RDebug::Print(_L("iBkBootRecSector   = %8d (0x%08X)"), BootSector.BkBootRecSector(), BootSector.BkBootRecSector() * BootSector.BytesPerSector());
        }
    }

GLDEF_C void DumpFat(const TUint8* aFat=NULL)
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

GLDEF_C TDes* DirAttributes(TInt aAttrib)
//
// Return a pointer to a local buffer containing the attribute letters.
//
    {
    LOCAL_D TBuf<6> str;
    LOCAL_D char*   atr = "RHSVDA";
    str.Fill(TText('-'), 6);
    for (TInt i = 0; i < 6; i++)
        if ((aAttrib >> i) & 1)
            str[i] = atr[i];
    return &str;
    }

GLDEF_C TBool IsValidDirEntry(TFatDirEntry* aDir)
//
// Test whether buffer is a valid normal directory entry
//
    {
    // first character must be 0x05 or greater than space
    if (aDir->iData[0] < 0x21 && aDir->iData[0] != 0x05)
        return EFalse;
    // other characters must be not less than space
    for (TInt i = 1; i < 11; i++)
        if (aDir->iData[i] < 0x20)
            return EFalse;
    return ETrue;
    }

GLDEF_C void GetLongNamePart(TDes16& aName, const TUint8* aEntry, TInt aPos, TInt aOffset, TInt aLength)
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

GLDEF_C void ExtractNameString(TDes16& aName, const TUint8* aEntry)
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

TDes& MakePrintable(TDes& aDes)
	{
	TInt len = aDes.Length();

	for (TInt i=0; i<len; i++)
		{
		if ((TUint8) aDes[i] < 0x20)
			aDes[i] = '?';
		}
	return aDes;
	}

GLDEF_C TBool DumpDirEntry(TInt aNum, const TUint8* aEntry)
//
// Dump a single directory entry to the log.  Return false if it was end of
// directory or an invalid entry (and don't display it).
//
    {
    TFatDirEntry* d = (TFatDirEntry*)aEntry;
    if (d->IsErased())
        {
        //RDebug::Print(_L("%5d: ERASED"), aNum);
        }
    else if (d->IsEndOfDirectory())
        {
        if (aNum > 0)
            RDebug::Print(_L("%5d: ------------- end of directory"), aNum);
        return EFalse;
        }
    else if (((TInt)d->Attributes() & KDirAttrLongMask) == KDirAttrLongName)
        {
        TBuf16<15> name;
        ExtractNameString(name, aEntry);
        TInt ord = aEntry[0];
        if (ord & KDirLastLongEntry)
            RDebug::Print(_L("%5d: %-15S #%-2d LAST"), aNum, &MakePrintable(name), ord & ~KDirLastLongEntry);
        else
            RDebug::Print(_L("%5d: %-15S #%-2d"), aNum, &MakePrintable(name), ord & ~KDirLastLongEntry);
        }
    else if (!IsValidDirEntry(d))
        {
        if (aNum > 0)
            RDebug::Print(_L("%5d: ============= INVALID ENTRY"), aNum);
        return EFalse;
        }
    else
        {
        TBuf<11> name;
        name.Copy(d->Name());
        RDebug::Print(_L("%5d: '%S'   %S  start %-5d size %d"),
                      aNum, &MakePrintable(name), DirAttributes(d->Attributes()), d->StartCluster(), d->Size());
        }
    return ETrue;
    }

GLDEF_C void DumpDirCluster(const TUint8* aData, TInt aCluster=0)
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

GLDEF_C void DumpRootDir(const TUint8* aData)
//
// Dump the data area buffer, trying to interpret directory clusters (only look
// at clusters which are marked as 'used' in the FAT).
//
    {
    RDebug::Print(_L("Root dir @ 0x%08X:"), gRootDirStart);
    for (TInt i = 0; i < BootSector.RootDirEntries(); i++)
        {
        if (DumpDirEntry(i, aData))
            aData += KSizeOfFatDirEntry;
        else
            break;
        }
    }

GLDEF_C void DumpData(const TUint8* aFat, const TUint8* aDir)
//
// Dump the data area buffer, trying to interpret directory clusters (only look
// at clusters which are marked as 'used' in the FAT).
//
    {
    RDebug::Print(_L("--------------- DATA AREA ------------------"));
    if (gDiskType != EFat32)
        {
        DumpRootDir(aDir);
        }
    TInt max = (gFatTestEntries < gMaxDataCluster ? gFatTestEntries : gMaxDataCluster);
    for (TInt cluster = 2; cluster < max; cluster++)
        {
        if (GetFatEntry(cluster, aFat) != 0)
            {
            RDebug::Print(_L("Cluster %d @ 0x%08X:"), cluster, ClusterToByte(cluster));
            DumpDirCluster(aDir+gDataStartBytes-gRootDirStart, cluster);
            }
        }
    RDebug::Print(_L("--------------------------------------------"));
    }

GLDEF_C void DumpData(const TUint8* aFat, TInt aStart, TInt aEnd = -1)
//
// Dump clusters from disk (allows dumping of clusters not in our buffers).
// Only look at clusters marked as 'used' in the FAT.  Note that if aFat is
// NULL the FAT entries will also be read from disk (slower but allows for ones
// outside our copy in memory).
//
    {
    if (aStart > gFatTestEntries)
        return;
    RDebug::Print(_L("--------------- DATA AREA ------------------"));
    if (aEnd > gFatTestEntries)
        aEnd = gFatTestEntries;
    if (aEnd < 0)
        aEnd = aStart + 1;
    if (aStart < 2 && gDiskType != EFat32)
        {
        HBufC8* buf=HBufC8::New(BootSector.RootDirEntries() * KSizeOfFatDirEntry);
        test_NotNull(buf);
        TPtr8 ptr=buf->Des();
        TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
        test_KErrNone(r);
        r=TheRawDisk.Read(gRootDirStart, ptr);
        test_KErrNone(r);
        TheRawDisk.Close();
        DumpRootDir(buf->Ptr());
        delete(buf);
        aStart = 2;
        }
    for (TInt cluster = aStart; cluster < aEnd; cluster++)
        {
        if (GetFatEntry(cluster, aFat) != 0)
            {
            HBufC8* buf=HBufC8::New(gBytesPerCluster);
            test_NotNull(buf);
            TPtr8 ptr=buf->Des();
            TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
            test_KErrNone(r);
            r=TheRawDisk.Read(ClusterToByte(cluster), ptr);
            test_KErrNone(r);
            TheRawDisk.Close();
            RDebug::Print(_L("Cluster %d @ 0x%08X:"), cluster, ClusterToByte(cluster));
            DumpDirCluster(ptr.Ptr());
            delete buf;
            }
        }
    RDebug::Print(_L("--------------------------------------------"));
    }

GLDEF_C void DumpHex(const TUint8* aData, TInt aLen, TInt aBase=0)
//
// Dump a block of memory to the log in hex.
//
    {
    for (TInt base = 0; base < aLen; base += 16)
        {
        TBuf<16*3+3+16+1> buf;
        TInt off;
        for (off = base; off < aLen && off < base + 16; off++)
            {
            buf.Append(TText(' '));
            buf.AppendNumFixedWidth(aData[off], EHex, 2);
            }
        buf.Append(_L("  |"));
        for (off = base; off < aLen && off < base + 16; off++)
            {
            TUint8 ch = aData[off];
            buf.Append(ch < 0x20 || ch > 0x7E ? TText('_') : TText(ch));
            }
        buf.Append(_L("|"));
        RDebug::Print(_L("%04X: %S"), base+aBase, &buf);
        }
    }

GLDEF_C void DumpHex(const TPtrC8& aData, TInt aLen, TInt aBase=0)
//
// Dump a block of memory to the log in hex.
//
    {
    DumpHex(aData.Ptr(), aLen, aBase);
    }

GLDEF_C void DumpHex(TInt aPos, TInt aLen, TInt aBase=0)
//
// Dump a block of memory to the log in hex.
//
    {
    TPtr8 dirBuf=DirBufPtr->Des();
    DumpHex(dirBuf.Ptr()+aPos, aLen, aBase);
    }

GLDEF_C void Dump(TEntryInfo& aEntry)
//
// Dump an entry description to the log in hex
//
    {
    RDebug::Print(_L("--- TEntryInfo 0x%08X, %d"), aEntry.iBytePos, aEntry.iLength);
    TInt len = aEntry.iLength*KSizeOfFatDirEntry;
    DumpHex(aEntry.iBytePos, len, aEntry.iBytePos);
    }

LOCAL_C TInt GetStartCluster(TInt aCluster, TInt aEntry)
//
// Get the start cluster pertaining to a directory entry in a specific
// directory cluster, return -1 if not available (invalid entry).
//
    {
    HBufC8* buf=HBufC8::New(gBytesPerCluster*2);
    test_NotNull(buf);
    TPtr8 ptr=buf->Des();
    TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);
    r=TheRawDisk.Read(ClusterToByte(aCluster), ptr);
    test_KErrNone(r);
    TheRawDisk.Close();
    RDebug::Print(_L("Cluster %d @ 0x%08X:"), aCluster, ClusterToByte(aCluster));
    TFatDirEntry* d = (TFatDirEntry*)ptr.Ptr() + aEntry;
    while (((TInt)d->Attributes() & KDirAttrLongMask) == KDirAttrLongName && aEntry < gEntriesPerCluster)
        {
        if (d->IsErased() || d->IsEndOfDirectory())
            break;
        ++aEntry;
        d = (TFatDirEntry*)ptr.Ptr() + aEntry;
        }
    TInt start = d->StartCluster();
    if (d->IsErased())
        start = -1;
    else if (d->IsEndOfDirectory())
        start = -1;
    else if (((TInt)d->Attributes() & KDirAttrLongMask) == KDirAttrLongName)
        start = -1;
    else if (!IsValidDirEntry(d))
        start = -1;
    delete buf;
    return start;
    }

LOCAL_C void Format()
    {
    TInt nRes;

#if 0
    TFatFormatParam fmt;
    fmt.iFatType = EFat32;
    fmt.iSecPerCluster = 1;

    nRes = FormatFatDrive(TheFs, CurrentDrive(), ETrue, &fmt);
#else
    nRes = FormatFatDrive(TheFs, CurrentDrive(), ETrue);
#endif

    test_KErrNone(nRes);

    }

LOCAL_C void CreateDeepDir(TFileName& aDir,TInt aDepth)
//
// Increase directory strucutre by aDepth starting with aDir.
//
    {
    TFileName num;
    num.Num(1);
    num+=_L("\\");
    TInt r;
    while(aDepth--)
        {
        num[0] = TText(aDepth % 26 + 'A');
        aDir+=num;
        r=TheFs.MkDir(aDir);
        test_KErrNone(r);
        }
    }

LOCAL_C void DeleteDeepDir(TFileName& aDir,TInt aDepth)
//
// Delete dir structure.
//
    {
    TInt r;
    while(aDepth--)
        {
        r=TheFs.RmDir(aDir);
        test_KErrNone(r);
        aDir.SetLength(aDir.Length()-2);
        }
    }

LOCAL_C void CreateMaxDepthDir(TFileName& aDir1,TFileName& aDir2)
//
// Create directory structure with max possible depth-1.
// Achieved by using dir names of one character.
//
    {
    //create dir structure with depth of 25
    CreateDeepDir(aDir1,25);
    // split dir structure
    aDir2=aDir1;
    aDir2+=_L("a\\");
    TInt r=TheFs.MkDir(aDir2);
    test_KErrNone(r);
    // create dir with depth of 126 directories - one short of max depth
    CreateDeepDir(aDir1,101);
    // create dir with depth of 90
    CreateDeepDir(aDir2,64);
    }

LOCAL_C void DeleteMaxDepthDir(TFileName&aDir1,TFileName&aDir2)
//
// Deletes max depth dir structure.
//
    {
    DeleteDeepDir(aDir2,64);
    TInt r=TheFs.RmDir(aDir2);
    test_KErrNone(r);
    aDir2.SetLength(aDir2.Length()-2);
    DeleteDeepDir(aDir1,102);
    DeleteDeepDir(aDir1,24);
    }

LOCAL_C void MakeVeryLongName(TFileName& aLong)
//
// appends a very long file name to aLong
//
    {
    // create a name to take up 18 vfat entries - (1 sector + 2 entries)
    for(TInt i=0;i<234;++i)
        {
        TInt c='a'+i%26;
        aLong.Append(c);
        }
    }

GLDEF_C void CreateLongNames(TFileName& aLong, TInt aClusters)
//
// Creates entries to fill up the number of directory clusters
//
    {
    TInt len = aLong.Length(); // length of directory prefix
    MakeVeryLongName(aLong);
    TInt count = 0;
    RFile temp;
    for (TInt i = 0; i < aClusters * gEntriesPerCluster; i += 19)
        {
        aLong[len+0] = TText(count/26 + 'A');
        aLong[len+1] = TText(count%26 + 'A');
        count++;
        TInt r=temp.Create(TheFs,aLong,EFileShareAny);
        test_KErrNone(r);
        temp.Close();
        }
    }

GLDEF_C void DeleteLongNames(TFileName& aLong, TInt aClusters)
//
// Deletes entries created by CreateLongNames()
//
    {
    TInt len = aLong.Length(); // length of directory prefix
    MakeVeryLongName(aLong);
    TInt count = 0;
    for (TInt i = 0; i < aClusters * gEntriesPerCluster; i += 19)
        {
        aLong[len+0] = TText(count/26 + 'A');
        aLong[len+1] = TText(count%26 + 'A');
        count++;
        TInt r=TheFs.Delete(aLong);
        test_Value(r, r==KErrNone || r==KErrNotFound);
        }
    }

LOCAL_C void DeleteRootDir(TInt aNum=0)
//
// Delete all entries in the root directory up to the last - aNum
//
    {
    test.Next(_L("Delete Root Directory Entries"));
    TInt maxRootEntries = gRootDirEntries;
    TFileName name=_L("\\???xxx");
    TInt count=0;
    TInt entriesSoFar=2;
    TInt r;
    count = 0;
    for (entriesSoFar=0; entriesSoFar<maxRootEntries - aNum; entriesSoFar+=2)
        {
        name[1]=(TUint16)(count/26/26+'a');
        name[2]=(TUint16)(count/26%26+'a');
        name[3]=(TUint16)(count%26+'a');
        r=TheFs.Delete(name);
        test_Value(r, r==KErrNone || r==KErrNotFound);
        ++count;
        }
    }

LOCAL_C void CreateRootDir()
//
// fill up root directory to 1 clusters by creating entries and then deleting
// them except the last.
//
    {
    test.Next(_L("Create Root Directory Entries"));
    TInt maxRootEntries = gRootDirEntries;
    TFileName name=_L("\\???xxx");
    TInt count=0;
    TInt entriesSoFar=2;
    TInt r;
    RFile f;
    for (entriesSoFar=0; entriesSoFar<maxRootEntries; entriesSoFar+=2)
        {
        name[1]=(TUint16)(count/26/26+'a');
        name[2]=(TUint16)(count/26%26+'a');
        name[3]=(TUint16)(count%26+'a');
        r=f.Create(TheFs, name, EFileWrite);
        test_KErrNone(r);
        f.Close();
        ++count;
        }
    DeleteRootDir(1);
    }

LOCAL_C void FillUpRootDir(TInt aFree=0)
//
// fill up root directory
//
    {
    TInt maxRootEntries = gRootDirEntries - aFree;
    TFileName dir=_L("\\??\\");
    TInt count=0;
    TInt entriesSoFar=2;
    TInt r;
    while(entriesSoFar<maxRootEntries)
        {
        dir[1]=(TUint16)(count/26+'a');
        dir[2]=(TUint16)(count%26+'a');
        r=TheFs.MkDir(dir);
        test_KErrNone(r);
        entriesSoFar+=2;
        ++count;
        }
    }

LOCAL_C void UnFillUpRootDir(TInt aFree=0)
//
// reverse changes from FillUpRootDir()
//
    {
    TFileName dir=_L("\\??\\");
    TInt entriesSoFar=gRootDirEntries-aFree;
    TInt count=0;
    TInt r;
    while(entriesSoFar>2)
        {
        dir[1]=TUint16(count/26+'a');
        dir[2]=TUint16(count%26+'a');
        r=TheFs.RmDir(dir);
        test_KErrNone(r);
        entriesSoFar-=2;
        ++count;
        }
    }

LOCAL_C void DeleteDirectoryStructure()
//
// deletes the directory structure
//
    {
    test.Next(_L("Delete Directory Structure"));
    TInt r=TheFs.RmDir(_L("\\scndrv\\dir2\\almostfull\\"));
    test_KErrNone(r);
    TInt entriesNeeded=(gEntriesPerCluster-2) / 2;  //7*2entries + . + .. = full sector
    for (TInt i = 0; i < entriesNeeded; i++)
        {
        TFileName file=_L("\\scndrv\\dir2\\full\\__a");
        file.AppendNum(i);
        r=TheFs.Delete(file);
        test_Value(r, r==KErrNone||r==KErrNotFound);
        }
    r=TheFs.RmDir(_L("\\scndrv\\dir2\\full\\"));
    test_KErrNone(r);
    r=TheFs.RmDir(_L("\\scndrv\\dir2\\"));
    test_KErrNone(r);
    TFileName veryLongName=(_L("\\scndrv\\dir1\\"));
    MakeVeryLongName(veryLongName);
    r=TheFs.Delete(veryLongName);
    test_KErrNone(r);
    r=TheFs.RmDir(_L("\\scndrv\\dir1\\"));
    test_KErrNone(r);
    r=TheFs.RmDir(_L("\\scndrv\\"));
    test_KErrNone(r);
    }

LOCAL_C void CreateDirectoryStructure()
//
// creates the directory structure
//
    {
    test.Next(_L("Create Directory Structure"));
    // cluster 3 (root dir is cluster 2)
    TInt r=TheFs.MkDir(_L("\\scndrv\\"));
    test_KErrNone(r);
    // cluster 4
    r=TheFs.MkDir(_L("\\scndrv\\dir1\\"));
    test_KErrNone(r);
    TFileName veryLongName=(_L("\\scndrv\\dir1\\"));
    MakeVeryLongName(veryLongName);
    RFile f;
    // cluster 5
    r=f.Create(TheFs,veryLongName,EFileShareAny);
    test_KErrNone(r);
    r=f.SetSize(512);
    test_KErrNone(r);
    f.Close();
    // cluster 6
    r=TheFs.MkDir(_L("\\scndrv\\dir2\\"));
    test_KErrNone(r);
    // cluster 7
    r=TheFs.MkDir(_L("\\scndrv\\dir2\\full\\"));
    test_KErrNone(r);
    // cluster 8
    r=TheFs.MkDir(_L("\\scndrv\\dir2\\somedirwith3entries\\"));
    test_KErrNone(r);
    // cluster 9
    r=TheFs.MkDir(_L("\\scndrv\\dir2\\somedir2with3entries\\"));
    test_KErrNone(r);
    // cluster 10
    r=TheFs.MkDir(_L("\\scndrv\\dir2\\almostfull\\"));
    test_KErrNone(r);
    // cluster 11-17
    TInt entriesNeeded=(gEntriesPerCluster-2) / 2;  //7*2entries + . + .. = full sector
    for (TInt i = 0; i < entriesNeeded; i++)
        {
        TFileName file=_L("\\scndrv\\dir2\\full\\__a");
        file.AppendNum(i);
        LastInFull = file;
        r=f.Create(TheFs,file,EFileShareAny);
        test_KErrNone(r);
        if (i < 7)
            {
            r=f.SetSize(512);
            test_KErrNone(r);
            }
        f.Close();
        }
    // cluster 18-19
    TInt charLength=13*4+1; // name to take up 6 entries
    TFileName file1=_L("\\scndrv\\dir2\\almostfull\\");
    while(charLength--)
        {
        TInt c='A'+charLength%26;
        file1.Append(c);
        }
    TFileName file2=file1;
    file1.AppendNum(1);
    file2.AppendNum(2);
    r=f.Create(TheFs,file1,EFileShareAny);
    test_KErrNone(r);
    r=f.SetSize(512);
    test_KErrNone(r);
    f.Close();
    r=f.Create(TheFs,file2,EFileShareAny);
    test_KErrNone(r);
    r=f.SetSize(512);
    test_KErrNone(r);
    f.Close();
    }

LOCAL_C TUint8* DirPtr(TInt aOffset)
//
// Return a pointer to the offset in the dir buffer, or in the special
// extension buffer if the dir buffer isn't large enough.  If the extension
// buffer is needed, it is read from disk when it is created (2 clusters, to
// allow for entries spanning cluster boundaries).  Errors will occur if
// another cluster is needed before the extension has been released.
//
// Note that if an extension buffer is allocated then writing a dir buffer to
// disk will also write the extension buffer and any changes made in it.
//
    {
    // if the offset is in store, return its byte address
    if (aOffset < DirBufPtr->Des().MaxLength())
        return (TUint8*)DirBufPtr->Ptr() + aOffset;
    // not in main buffer, see if extension is allocated already
    if (!ExtBufPtr)
        {
        // allocate buffer for 2 clusters, starting at the cluster which
        // contains aOffset
        ExtBufLen = 2 * gBytesPerCluster;
        ExtBufPtr = HBufC8::New(ExtBufLen);
        test_NotNull(ExtBufPtr);
        // read the clusters in
        ExtBufAdd = aOffset - aOffset % gBytesPerCluster;
        TInt clust = (ExtBufAdd - (gDataStartBytes - gRootDirStart)) /gBytesPerCluster + 2;
        RDebug::Print(_L("Extension buffer for cluster %d allocated"), clust);
        TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
        test_KErrNone(r);
        TPtr8 des(ExtBufPtr->Des());
        r=TheRawDisk.Read(gRootDirStart + ExtBufAdd, des);
        test_KErrNone(r);
        TheRawDisk.Close();
        }
    // convert to offset in the extension buffer
    aOffset -= ExtBufAdd;
    if (aOffset >= ExtBufLen)
        {
        test.Printf(_L("*** ERROR: tried to access cluster %d not in store\n"),
                    (aOffset + ExtBufAdd) / gBytesPerCluster + 2);
        test(0);
        }
    return (TUint8*)ExtBufPtr->Ptr() + aOffset;
    }

LOCAL_C void DirPtrFree()
//
// Free the extension buffer and zero the references to it.
//
    {
    if (ExtBufPtr)
        {
        TInt clust = (ExtBufAdd - (gDataStartBytes - gRootDirStart)) /gBytesPerCluster + 2;
        RDebug::Print(_L("Extension buffer for cluster %d deleted"), clust);
        delete ExtBufPtr;
        ExtBufPtr = NULL;
        ExtBufAdd = 0;
        ExtBufLen = 0;
        }
    }

LOCAL_C void ReadDirDisk(TDes8& aDirBuf, TInt aCluster = -1)
//
// reads directory section of disk into buffer
//
    {
    test_Value(aCluster, aCluster != 1);

    TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);

    if (aCluster == -1) // all clusters ?
        {
        r=TheRawDisk.Read(gRootDirStart, aDirBuf);
        }
    else if (aCluster == 0) // root directory cluster
        {
        TPtr8 dirPtr = aDirBuf.MidTPtr(0, gBytesPerCluster);
        r=TheRawDisk.Read(gRootDirStart, dirPtr);
        }
    else
        {
        // directory buffer starts at root directory, so
        // calculate offset from there.
        TInt pos = ((aCluster - 2) * gBytesPerCluster) +
            (gRootDirSectors * BootSector.BytesPerSector());
        TPtr8 dirPtr = aDirBuf.MidTPtr(pos, gBytesPerCluster);
        r=TheRawDisk.Read(gRootDirStart + pos, dirPtr);
        }

    test_KErrNone(r);
    TheRawDisk.Close();
    }

LOCAL_C void ReadFatDisk(TDes8& aFatBuf)
//
// reads fat section of disk info buffer
//
    {
    TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);
    r=TheRawDisk.Read(gFatStartBytes, aFatBuf);
    test_KErrNone(r);
    TheRawDisk.Close();
    }

LOCAL_C void WriteDirEntryToDisk(TDes8& aDirBuf, TInt aCluster = -1)
//
// writes dir entry buffer to disk
//
    {
	test_Value(aCluster, aCluster != 1);

    TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);

    if (aCluster == -1)
        {
        r=TheRawDisk.Write(gRootDirStart, aDirBuf);
        }
    else if (aCluster == 0) // root directory cluster
        {
        TPtr8 dirPtr = aDirBuf.MidTPtr(0, gBytesPerCluster);
        r=TheRawDisk.Write(gRootDirStart, dirPtr);
        }
    else
        {
        // directory buffer starts at root directory, so
        // calculate offset from there.
        TInt pos = ((aCluster - 2) * gBytesPerCluster) +
            (gRootDirSectors * BootSector.BytesPerSector());
        TPtr8 dirPtr = aDirBuf.MidTPtr(pos, gBytesPerCluster);
        r=TheRawDisk.Write(gRootDirStart + pos, dirPtr);
        }

    test_KErrNone(r);
    if (ExtBufPtr)
        {
        TPtr8 des(ExtBufPtr->Des());
        r=TheRawDisk.Write(gRootDirStart + ExtBufAdd, des);
        test_KErrNone(r);
        }
    TheRawDisk.Close();
    }

LOCAL_C void WriteFatToDisk(TDes8& aFatBuf, TInt aStart=0)
//
// writes fat buffer to disk
//
    {
    TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);
    TInt fatCount=BootSector.NumberOfFats() - aStart;
    TInt pos = gFatStartBytes + aStart * gFatSizeSectors*BootSector.BytesPerSector();
    while(fatCount--)
        {
        r=TheRawDisk.Write(pos, aFatBuf);
        test_KErrNone(r);
        pos += gFatSizeSectors*BootSector.BytesPerSector();
        }
    TheRawDisk.Close();
    }

LOCAL_C void WriteReservedId(TInt aBytePos)
//
// write reserved id to byte 19 of entry starting at aBytePos
//
    {
    TUint8* pEntry=DirPtr(aBytePos);
    pEntry[19]=1;
    }

LOCAL_C void WriteEndOfDir(TInt aBytePos)
//
// write end of directory marker to entry starting at aBytePos
//
    {
    TUint8* pEntry=DirPtr(aBytePos);
    Mem::FillZ(pEntry,KFatDirNameSize);
    }

LOCAL_C void WriteDelete(TInt aBytePos,TInt aNum)
//
// writes 0xe5 to entries starting at aBytePos
//
    {
    TUint8* pEntry=DirPtr(aBytePos);
    while(aNum--)
        {
        pEntry[0]=0xE5;
        pEntry+=KSizeOfFatDirEntry;
        }
    }

LOCAL_C void WriteCopyDir(TInt aSrc, TInt aDst)
//
// Copy one directory entry over another
//
    {
    TUint8* pSrc=DirPtr(aSrc);
    TUint8* pDst=DirPtr(aDst);
    Mem::Copy(pDst, pSrc, KSizeOfFatDirEntry);
    }

LOCAL_C void InitialiseBuffers()
//
// reads disk into buffers
//
    {
    gFatTestEntries = MaxClusters();
    if (gFatTestEntries > KMaxFatSize)
        gFatTestEntries = KMaxFatSize;
    gFatTestSize = PosInBytes(gFatTestEntries);
    FatBufPtr=HBufC8::New(gFatTestSize);
    test_NotNull(FatBufPtr);
    DirBufPtr=HBufC8::New(DirBufferSize());
    test_NotNull(DirBufPtr);

    // Buffers for reading from disk
    FatDiskPtr=HBufC8::New(gFatTestSize);
    test_NotNull(FatDiskPtr);
    DirDiskPtr=HBufC8::New(DirBufferSize());
    test_NotNull(DirDiskPtr);
    }

LOCAL_C TBool IsSameAsDrive(const TDes8& aFatBuf,const TDes8& aDirBuf)
//
// compares the two bufs passed in with those on disk
//
    {
    TPtr8 fatDisk=FatDiskPtr->Des();
    TPtr8 dirDisk=DirDiskPtr->Des();
    ReadDirDisk(dirDisk);
    ReadFatDisk(fatDisk);
    TBool fatOk = (aFatBuf.Compare(fatDisk)==0);
    TBool dirOk = (aDirBuf.Compare(dirDisk)==0);
    if (!fatOk)
        {
        TInt i = FindUnMatch(aFatBuf.Ptr(), fatDisk.Ptr(), fatDisk.Length());
        switch (gDiskType)
            {
            case EFat32:
                i /= 4;

                if(i == 1)
                {//-- mismatch in FAT[1] for FAT16/FAT32 it is OK because FAT[1] is used by volume finalisation
                 //-- to store Volume Clean Shutdown flag
                    fatOk = ETrue;
                }

                break;

            case EFat16:
                i /= 2;

                if(i == 1)
                {//-- mismatch in FAT[1] for FAT16/FAT32  it is OK because FAT[1] is used by volume finalisation
                 //-- to store Volume Clean Shutdown flag
                    fatOk = ETrue;
                }

                break;

            case EFat12:
                i = i*2 / 3;
                if (GetFatEntry(i, aFatBuf.Ptr()) == GetFatEntry(i, fatDisk.Ptr()))
                    ++i;
                break;
            default:
                test(0);
            }

        if(fatOk && i==1)
            {
            test.Printf(_L("Skipping FAT[1] entry for FAT16/32 \n"), i);
            }
        else
            {
        test.Printf(_L("FAT entry %d different from expected\n"), i);

        RDebug::Print(_L("Expected:\n"));
        DumpFat(aFatBuf.Ptr());
        RDebug::Print(_L("Actual:\n"));
        DumpFat(fatDisk.Ptr());
        }
        }

    if (!dirOk)
        {
        TInt i = FindUnMatch(aDirBuf.Ptr(), dirDisk.Ptr(), dirDisk.Length());
        TInt clust = ByteToCluster(i);
        TInt entry = i % gBytesPerCluster / KSizeOfFatDirEntry;
        test.Printf(_L("DIR different from expected\n"));
        test.Printf(_L("  at pos %d sector %d cluster %d entry %d:\n"), i, i / BootSector.BytesPerSector(), clust, entry);

		RDebug::Print(_L("Expected:\n"));
		DumpHex(aDirBuf.Ptr() + i, 32);
		RDebug::Print(_L("-------------"));
		RDebug::Print(_L("Actual:\n"));
		DumpHex(dirDisk.Ptr() + i, 32);

		RDebug::Print(_L("Expected:\n"));
		DumpData(aFatBuf.Ptr(), aDirBuf.Ptr());
		RDebug::Print(_L("Actual:\n"));
		DumpData(fatDisk.Ptr(), dirDisk.Ptr());
        }
    else if (ExtBufPtr)
        {
        HBufC8* extPtr = HBufC8::New(ExtBufLen);
        test_NotNull(extPtr);
        TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
        test_KErrNone(r);
        TPtr8 des(extPtr->Des());
        r=TheRawDisk.Read(ExtBufAdd+gRootDirStart, des);
        test_KErrNone(r);
        TheRawDisk.Close();
        TInt i = FindUnMatch(ExtBufPtr->Ptr(), extPtr->Ptr(), ExtBufLen);
        if (i >= 0)
            {
            TInt extcl = (ExtBufAdd - (gDataStartBytes-gRootDirStart)) / gBytesPerCluster + 2;
            TInt clust = i / gBytesPerCluster;
            TInt entry = i % gBytesPerCluster / KSizeOfFatDirEntry;
            test.Printf(_L("DIR different from expected\n"));
            test.Printf(_L("  at cluster %d entry %d:\n"), extcl+clust, entry);
            DumpHex(ExtBufPtr->Ptr() + clust * gBytesPerCluster + entry * KSizeOfFatDirEntry, 32);
            RDebug::Print(_L("-------------"));
            DumpHex(extPtr->Ptr() + clust * gBytesPerCluster + entry * KSizeOfFatDirEntry, 32);
            // RDebug::Print(_L("Expected:\n"));
            // DumpData(aFatBuf.Ptr(), aDirBuf.Ptr());
            // RDebug::Print(_L("Actual:\n"));
            // DumpData(fatDisk.Ptr(), dirDisk.Ptr());
            dirOk = EFalse;
            }
        delete extPtr;
        }

    return(fatOk && dirOk);
    }

LOCAL_C void WriteErased(TEntryInfo aTrg,TInt aToDelete)
//
// writes erased marker, starting at dos entry and working backwards
// used to simulate a part entry40*BootSector.BytesPerSector()
//
    {
    TInt toStart=aTrg.iBytePos+(aTrg.iLength-1)*KSizeOfFatDirEntry;
    TPtr8 dirBuf=DirBufPtr->Des();
    while(aToDelete--)
        {
        dirBuf[toStart]=0xE5;
        toStart-=KSizeOfFatDirEntry;
        }
    }

LOCAL_C void CreatePartialEntry(TEntryInfo aTrg,TInt aToDelete,TBool aAddEOfDir)
//
// creates a partial entry
//
    {
    WriteErased(aTrg,aToDelete);
    if(aAddEOfDir)
        WriteEndOfDir(aTrg.iBytePos+aTrg.iLength*KSizeOfFatDirEntry);
    TPtr8 dirBuf=DirBufPtr->Des();
    WriteDirEntryToDisk(dirBuf);
    }

LOCAL_C TBool TestPartialEntry(TEntryInfo aEntry)
//
// tests that scandrive deals with a partial entry and returns the result
//
    {
    test.Next(_L("TestPartialEntry"));
    TInt r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    WriteDelete(aEntry.iBytePos,aEntry.iLength);

    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();

    TBool res=IsSameAsDrive(fatBuf,dirBuf);
    return(res);
    }

LOCAL_C void CreateMatchingEntry(TEntryInfo aTrg,TEntryInfo aSrc,TBool aAddEOfDir)
//
// creates matching entry
//
    {
    test.Next(_L("Create entry with start cluster already used"));
    TUint8* src = DirPtr(aSrc.iBytePos);
    TUint8* dst = DirPtr(aTrg.iBytePos);
    Mem::Copy(dst, src, aSrc.iLength*KSizeOfFatDirEntry);
    WriteReservedId(aTrg.iBytePos+(aTrg.iLength-1)*KSizeOfFatDirEntry);
    if(aAddEOfDir)
        WriteEndOfDir(aTrg.iBytePos+aTrg.iLength*KSizeOfFatDirEntry);
    TPtr8 dirBuf=DirBufPtr->Des();
    WriteDirEntryToDisk(dirBuf);
    }

LOCAL_C TBool TestMatchingEntry(TEntryInfo aToDelete)
//
// tests that scandrive deals with matching entries correctly
//
    {
    test.Next(_L("TestMatchingEntries"));
    WriteDelete(aToDelete.iBytePos,aToDelete.iLength);
    TInt r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);

    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();

    TBool res=IsSameAsDrive(fatBuf,dirBuf);
    DirPtrFree();
    return(res);
    }


LOCAL_C void TestExtendedChars()
//
// tests that extended characters corresponding to ISO Latin 1
// characters 128-255 are recognised as valid by scandrive
//
    {
    test.Next(_L("TestExtendedChars()"));
    Format();

    _LIT(KRoot,"\\");
    CDir* dirs;
    // check no entries in the root directory
    TInt r=TheFs.GetDir(KRoot,KEntryAttMaskSupported,ESortNone,dirs);
    test_KErrNone(r);
    test_Equal(0,dirs->Count());
    delete(dirs);
    dirs=NULL;

    // create file
    _LIT(KOrigShortName,"P_SSI.TXT");

    //_LIT(KTestFile,"\\p\xE4ssi.txt"); //-- this causes problems for VC6 and default locale different from English
    TBuf<64> TestFileName(_L("\\p$ssi.txt"));
    TestFileName[2] = 0xe4; //-- replace '$' with this code

    //_LIT(KExtShortName,"P\xC4SSI.TXT"); //-- this causes problems for VC6 and default locale different from English
    TBuf<64> ExtShortName(_L("P$SSI.TXT"));
    ExtShortName[1] = 0xC4; //-- replace '$' with this code


    RFile file;
    r=file.Replace(TheFs,TestFileName,EFileShareExclusive);
    test_KErrNone(r);
    file.Close();

    // get short name
    TFileName shortName;
    r=TheFs.GetShortName(TestFileName,shortName);
    test_KErrNone(r);
    test(shortName==KOrigShortName);

    // must be first entry in root, modify to read like
    // a windows-generated short name (ie. contains extended character)
    DumpData(NULL, 0, 20);
    TInt bytePos=ClusterEntryToBytes(0,1);
    RRawDisk raw;
    r=raw.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);
    TBuf8<1> buf(1);

    //-- change 2nd character in the short name (Fat DOS entry)
    buf[0]=(TUint8)'\xC4';
    r=raw.Write(gRootDirStart+bytePos+1,buf);
    test_KErrNone(r);

    //-- fix the fiddled short name checksum in the corresponding VFat entry
    bytePos=ClusterEntryToBytes(0,0);
    buf[0]=(TUint8)0x2f;
    r=raw.Write(gRootDirStart+bytePos+13,buf);
    test_KErrNone(r);

    // retrieve short name from media.
    // Note: do not use RFs::GetShortName() as its behaviours are code page dependent.
    bytePos=ClusterEntryToBytes(0,1);
    TBuf8<11> shortNameBuf8;
    r=raw.Read(gRootDirStart+bytePos,shortNameBuf8);
    test_KErrNone(r);
    shortNameBuf8 = DosNameFromStdFormat(shortNameBuf8);
    shortName.Copy(shortNameBuf8);
    raw.Close();


    test(shortName==ExtShortName);
    DumpData(NULL, 0, 20);
    //TheFs.SetDebugRegister(KFSYS);
    r=TheFs.ScanDrive(gSessionPath);
    TheFs.SetDebugRegister(0);
    test_KErrNone(r);
    DumpData(NULL, 0, 20);

    // retrieve short name from media.
    r=raw.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);
    bytePos=ClusterEntryToBytes(0,1);
    r=raw.Read(gRootDirStart+bytePos,shortNameBuf8);
    test_KErrNone(r);
    shortNameBuf8 = DosNameFromStdFormat(shortNameBuf8);
    shortName.Copy(shortNameBuf8);
    raw.Close();

    test(shortName==ExtShortName);

    // delete file
    r=TheFs.Delete(TestFileName);
    test_KErrNone(r);
    }

LOCAL_C void TestMountAndScan()
//
// test MountFileSystemAndScan()
//
    {
    TFullName extName;
    TBool primaryExtensionExists = EFalse;

    test.Next(_L("TestMountAndScan"));
    HBufC8* newFat=HBufC8::New(gFatTestSize);
    test_NotNull(newFat);
    TPtr8 fat=newFat->Des();
    TPtr8 origFat=FatBufPtr->Des();
    TPtr8 origDir=DirBufPtr->Des();

    // set cluster of \scndrv\dir1\ to a hanging cluster
    ReadFatDisk(fat);
    WriteToFatBuf(gClusterDir1ext,35,fat.Ptr());
    WriteToFatBuf(35,36,fat.Ptr());
    WriteFatToDisk(fat);
    // set the default path to something other than the current drive
    TFileName fsName;
    TInt r=TheFs.FileSystemName(fsName,gSessionPath[0]-'A');
    test_KErrNone(r);
    TFileName origDefPath, newDefPath;
    r=TheFs.SessionPath(origDefPath);
    test_KErrNone(r);
    newDefPath=origDefPath;
    newDefPath[0]=(TText)'z';
    r=TheFs.SetSessionPath(newDefPath);
    test_KErrNone(r);
    r = TheFs.ExtensionName(extName,gSessionPath[0]-'A',0);
    if (r == KErrNone)
        {
        primaryExtensionExists = ETrue;
        }
    r=TheFs.DismountFileSystem(fsName,gSessionPath[0]-'A');
    test_KErrNone(r);
    // mount file system and check scandrive corrects error
    TBool isMount;
    if (primaryExtensionExists)
        r=TheFs.MountFileSystemAndScan(fsName,extName,gSessionPath[0]-'A',isMount);
    else
        r=TheFs.MountFileSystemAndScan(fsName,gSessionPath[0]-'A',isMount);
    test(isMount);
    test_KErrNone(r);
    TBool res=IsSameAsDrive(origFat,origDir);
    test(res);

    r=TheFs.SetSessionPath(origDefPath);
    test_KErrNone(r);
    delete newFat;
    }


LOCAL_C void TestConsecutiveMountAndScans()
//
// test fix for DEF093072: [codebase]MountFileSystemAndScan returns err -21 but ok flag
//
    {
    TFullName extName;
    TBool primaryExtensionExists = EFalse;
    TFileName fsName;
    TInt r=TheFs.FileSystemName(fsName,gSessionPath[0]-'A');
    test_KErrNone(r);
    r = TheFs.ExtensionName(extName,gSessionPath[0]-'A',0);
    if (r == KErrNone)
        {
        primaryExtensionExists = ETrue;
        }
    r=TheFs.DismountFileSystem(fsName,gSessionPath[0]-'A');
    test_KErrNone(r);

    // RFs::MountFileSystemAndScan twice consecutively
    // first time
    TBool isMount;
    if (primaryExtensionExists)
        r=TheFs.MountFileSystemAndScan(fsName,extName,gSessionPath[0]-'A',isMount);
    else
        r=TheFs.MountFileSystemAndScan(fsName,gSessionPath[0]-'A',isMount);
    test(isMount);
    test_KErrNone(r);
    // and a second time
    if (primaryExtensionExists)
        r=TheFs.MountFileSystemAndScan(fsName,extName,gSessionPath[0]-'A',isMount);
    else
        r=TheFs.MountFileSystemAndScan(fsName,gSessionPath[0]-'A',isMount);
    test(!isMount);
    test_Equal(KErrAccessDenied,r);
    }


static void CreateContiguousClusterChain(TUint32 aStartIndex, TUint32 aEndIndex, const TUint8* aFatPtr, TBool aMarkEoc)
/*
 * Creates a contiguous cluster chain in the FAT buffer.
 * 
 * @param	aStartIndex	The first cluster index of the chain
 * 			aEndIndex	The last cluster index of the chain 
 * 			aFatPtr		FAT table buffer pointer
 * 			aMarkEoc	If ETrue, aEndIndex will be marked as EOC, else it will be a hanging cluster chain
 */
	{
	// Write cluster chain
    for(TUint i=aStartIndex; i<aEndIndex; ++i)
        WriteToFatBuf(i, i+1, aFatPtr);
    // Mark EOC if needed
    if (aMarkEoc)
    	WriteToFatBuf(aEndIndex, gEndOfChain, aFatPtr);
	}


LOCAL_C void DoHangingClusters()
//
// Tests that scandrive removes hanging clusters
//
    {
    test.Next(_L("Check Hanging clusters"));
    HBufC8* newFat=HBufC8::New(gFatTestSize);
    test_NotNull(newFat);
    TPtr8 fat=newFat->Des();
    TPtr8 origFat=FatBufPtr->Des();
    TPtr8 origDir=DirBufPtr->Des();

    // Set cluster of \scndrv\dir1\ to hanging cluster chain
    test.Start(_L("Test hanging cluster in \\scndrv\\dir1\\"));
    ReadFatDisk(fat);
    WriteToFatBuf(gClusterDir1ext,35,fat.Ptr());
    WriteToFatBuf(35,36,fat.Ptr());
    WriteFatToDisk(fat); // gClusterDir1ext->35->36
    TInt r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    TBool res=IsSameAsDrive(origFat,origDir);
    test(res);

    // Set cluster chain of first entry of \scndrv\dir1\ to
    // larger size than file size
    test.Next(_L("Test hanging cluster in first entry"));
    ReadFatDisk(fat);
    WriteToFatBuf(gClusterDir1ext,39,fat.Ptr());
    WriteToFatBuf(39,500,fat.Ptr());
    CreateContiguousClusterChain(500, 505, fat.Ptr(), ETrue);
    WriteFatToDisk(fat); // gClusterDir1ext->39->500->501->502->503->504->505->EOC
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    res=IsSameAsDrive(origFat,origDir);
    test(res);

    // Set cluster of \scndrv\ to a hanging cluster
    test.Next(_L("Test hanging cluster of \\scndrv\\"));
    ReadFatDisk(fat);
    WriteToFatBuf(gClusterScnDrv,511,fat.Ptr());
    WriteFatToDisk(fat); // gClusterScnDrv->511
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    res=IsSameAsDrive(origFat,origDir);
    test(res);

    delete newFat;
    test.End();
    }

LOCAL_C void DoLostClusters()
//
// Tests that scandrive removes lost clusters
//
    {
    test.Next(_L("Check lost clusters"));
    HBufC8* newFat=HBufC8::New(gFatTestSize);
    test_NotNull(newFat);
    TPtr8 fat=newFat->Des();
    TPtr8 origFat=FatBufPtr->Des();
    TPtr8 origDir=DirBufPtr->Des();
    ReadFatDisk(origFat);
    ReadDirDisk(origDir);

    // write cluster chain
    test.Start(_L("Test removal of lost cluster chain"));
    ReadFatDisk(fat);
    CreateContiguousClusterChain(25, 35, fat.Ptr(), ETrue);
    WriteFatToDisk(fat); // 25->26->27->28->29->30->31->32->33->34->35->EOC
    TInt r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    TBool res=IsSameAsDrive(origFat,origDir);
    test(res);

    // write semi-random changes to first fat
    test.Next(_L("Test semi-random changes to first fat"));
    for(TInt j=1;j<gFatTestSize/BootSector.BytesPerSector();++j)
        {
        TInt off = j*BootSector.BytesPerSector()+j*7%512;
        fat[off]=1;
        }
    WriteFatToDisk(fat);
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    res=IsSameAsDrive(origFat,origDir);
    test(res);

    // write semi-random changes to second fat
    test.Next(_L("Test semi-random changes to second fat"));
    WriteFatToDisk(fat, 1);
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    res=IsSameAsDrive(origFat,origDir);
    test(res);

    delete newFat;
    test.End();
    }


static void DoHangingAndLostClusters()
/*
 * Tests that ScanDrive fixes MULTIPLE hanging clusters and removes lost clusters.
 * It creates multiple hanging and lost cluster chains in the FAT table and
 * expects ScanDrive to fix them all.
 */
	{
	test.Start(_L("Check multiple hanging and lost cluster chains"));
	HBufC8* newFat = HBufC8::New(gFatTestSize);
	test_NotNull(newFat);
	TPtr8 fat = newFat->Des();
	TPtr8 origFat = FatBufPtr->Des();
	TPtr8 origDir = DirBufPtr->Des();
	ReadFatDisk(origFat);
	ReadDirDisk(origDir);

	test.Printf(_L("Create multiple hanging cluster chains\n"));
	ReadFatDisk(fat);
	// Set hanging cluster for the file in \scndrv\dir1
	// gClusterDir1ext+1->25->26->27->28->29->30->31->32->33->34->35
	WriteToFatBuf(gClusterDir1ext+1, 25, fat.Ptr());
	CreateContiguousClusterChain(25, 35, fat.Ptr(), EFalse);
	// Set hanging cluster for the first file in \scndrv\dir2
	// gClusterDir2_AFull+1->249->250->53->54->55->EOC
	WriteToFatBuf(gClusterDir2_AFull+1, 249, fat.Ptr());
	WriteToFatBuf(249, 250, fat.Ptr());
	WriteToFatBuf(250, 53, fat.Ptr());
	CreateContiguousClusterChain(53, 55, fat.Ptr(), ETrue);
	// Set hanging cluster for the fourth file in \scndrv\dir2
	// gClusterDir2_AFull+4->59->60->61->62->63
	WriteToFatBuf(gClusterDir2_AFull+4, 59, fat.Ptr());
	CreateContiguousClusterChain(59, 63, fat.Ptr(), EFalse);
	// Set hanging cluster for the second file in \scndrv\dir2
	// gClusterDir2_AFull+2->67->68->69->EOC
	WriteToFatBuf(gClusterDir2_AFull+2, 67, fat.Ptr());
	CreateContiguousClusterChain(67, 69, fat.Ptr(), ETrue);

	test.Printf(_L("Create multiple lost clusters\n"));
	// Create 1st lost cluster chain (clusters 36-45)
	CreateContiguousClusterChain(36, 45, fat.Ptr(), ETrue);
	// Create 2nd lost cluster chain (clusters 246-248,56-58)
	CreateContiguousClusterChain(246, 248, fat.Ptr(), EFalse);
	WriteToFatBuf(248, 56, fat.Ptr());
	CreateContiguousClusterChain(56, 58, fat.Ptr(), ETrue);
	// Create 3rd lost cluster chain (clusters 251-253,564-566, with hanging end)
	CreateContiguousClusterChain(251, 253, fat.Ptr(), EFalse);
	WriteToFatBuf(253, 564, fat.Ptr());
	CreateContiguousClusterChain(564, 566, fat.Ptr(), EFalse);
	
	// Flush all FAT changes to the media
	WriteFatToDisk(fat);

	test.Next(_L("Test ScanDrive fixes multiple hanging and lost cluster chains"));
	TInt r = TheFs.CheckDisk(gSessionPath);	// CheckDisk should detect an error
	test_Value(r, r != KErrNone);
	r = TheFs.ScanDrive(gSessionPath);		// ScanDrive should find the errors and fix them
	test_KErrNone(r);
	r = TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	TBool res = IsSameAsDrive(origFat, origDir);
	test(res);

	delete newFat;
	test.End();
	}


LOCAL_C void DoPartEntries()
//
// Tests that scandrive detects/corrects partial entries
//
    {
    RFile temp;
    TInt last;
    TBool res;
    test.Start(_L("Check partial entries"));
    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();

    TInt r=TheFs.RmDir(_L("\\scndrv\\dir2\\somedirwith3entries\\"));
    test_Value(r, r==KErrNone || r==KErrNotFound || r==KErrPathNotFound);
    r=TheFs.RmDir(_L("\\scndrv\\dir2\\somedir2with3entries\\"));
    test_Value(r, r==KErrNone || r==KErrNotFound || r==KErrPathNotFound);

    if (BootSector.RootDirEntries() != 0)
        {
        // Can only do this on FAT12/16, FAT32 root directory is extensible
        // partial entry that fills up the root dir
        test.Next(_L("Partial entry at end of rootdir"));
        FillUpRootDir(2);
        r=temp.Create(TheFs,_L("\\temp"),EFileShareAny);
        test_KErrNone(r);
        temp.Close();
        ReadDirDisk(dirBuf);
        ReadFatDisk(fatBuf);
        TEntryInfo partial1(ClusterEntryToBytes(gClusterRootDir,BootSector.RootDirEntries()-2),2);
        CreatePartialEntry(partial1,1,EFalse);
        res=TestPartialEntry(partial1);
        test(res);
        UnFillUpRootDir(2);
        ReadDirDisk(dirBuf);
        ReadFatDisk(fatBuf);
        }

    // use first entry \scndrv\dir2\almostfull
    test.Next(_L("Partial entry in middle of subdir"));
    last = GetStartCluster(gClusterDir2_AFull,7);
    TEntryInfo partial2(ClusterEntryToBytes(gClusterDir2_AFull,2),6);
    CreatePartialEntry(partial2,3,EFalse);
    // entry has been allocated a cluster which scandrive should delete along with partial entry
    if (last > 0)
        WriteToFatBuf(last,0,fatBuf.Ptr());
    res=TestPartialEntry(partial2);
    test(res);

    // reduce size of \scndrv\dir2\full
    test.Next(_L("Test directory reclaim"));
    last = GetStartCluster(gClusterDir2_Full,gEntriesPerCluster-2);
    WriteEndOfDir(ClusterEntryToBytes(gClusterDir2_Full,gEntriesPerCluster-2));
    WriteDirEntryToDisk(dirBuf);
    TInt entry = GetFatEntry(gClusterDir2_Full, fatBuf.Ptr());
    WriteToFatBuf(gClusterDir2_Full,gEndOfChain,fatBuf.Ptr());
    while (entry && (entry & gEndOfChain) != gEndOfChain)
        {
        TInt next = GetFatEntry(entry, fatBuf.Ptr());
        WriteToFatBuf(entry,0,fatBuf.Ptr());
        entry = next;
        }
    if (last > 0)
        WriteToFatBuf(last,0,fatBuf.Ptr());
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    res=IsSameAsDrive(fatBuf,dirBuf);
    test(res);

    // use last entry of first cluster in \scndrv\dir2\full
    test.Next(_L("Partial entry at end of subdir"));
    r=temp.Create(TheFs,_L("\\scndrv\\dir2\\full\\temp"),EFileShareAny);
    test_KErrNone(r);
    temp.Close();
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);
    TEntryInfo partial3(ClusterEntryToBytes(gClusterDir2_Full,gEntriesPerCluster-2),2);
    CreatePartialEntry(partial3,1,EFalse);
    res=TestPartialEntry(partial3);
    test(res);

    // use entry in \scndrv\dir2\almostfull
    test.Next(_L("Partial entry preceeding end-of-dir marker"));
    last = GetStartCluster(gClusterDir2_AFull,14);
    if (last > 0)
        WriteToFatBuf(last,0,fatBuf.Ptr());
    last = GetStartCluster(gClusterDir2_AFull,8);
    if (last > 0)
        WriteToFatBuf(last,0,fatBuf.Ptr());
    WriteEndOfDir(ClusterEntryToBytes(gClusterDir2_AFull,14));
    WriteDirEntryToDisk(dirBuf);
    TEntryInfo partial4(ClusterEntryToBytes(gClusterDir2_AFull,8),6);
    CreatePartialEntry(partial4,4,EFalse);
    res=TestPartialEntry(partial4);
    test(res);

	// NOTE:
	// Following test case is not valid anymore after fixing of
	//	PDEF128576: Unicode name file deleted after Scandrive
	// In the fixes, we decided to discard file name checking in ScanDrive,
	//	as it is impossible for ScanDrive to judge if the illegal byte is part of a legal
	//	DBCS charater.

	// create entry in \scndrv\dir2\almostfull
//	test.Next(_L("Partial entry with invalid dos name"));
//	r=temp.Create(TheFs,_L("\\scndrv\\dir2\\almostfull\\Dodgy file name"),EFileShareAny);
//	test_KErrNone(r);
//	temp.Close();
//	ReadDirDisk(dirBuf);
//	TInt dosStart=ClusterEntryToBytes(gClusterDir2_AFull,4);
//	dirBuf[dosStart+4]=0x1;
//	WriteDirDisk(dirBuf);
//	r=TheFs.ScanDrive(gSessionPath);
//	test_KErrNone(r);
//	WriteDelete(dosStart-2*32,3);
//	res=IsSameAsDrive(fatBuf,dirBuf);
//	test(res);

    if (BootSector.SectorsPerCluster() == 1)
        {
        // use entry created in \scndrv\dir2
        test.Next(_L("Partial entry spanning more than two clusters"));
        last = GetStartCluster(gClusterDir2_Full,gEntriesPerCluster-1);
        WriteEndOfDir(ClusterEntryToBytes(gClusterDir2_Full,gEntriesPerCluster-2));
        WriteEndOfDir(ClusterEntryToBytes(gClusterDir2_Full,gEntriesPerCluster-1));
        WriteDirEntryToDisk(dirBuf);
        TFileName longFile=_L("\\scndrv\\dir2\\full\\");
        MakeVeryLongName(longFile);
        r=temp.Create(TheFs,longFile,EFileShareAny);
        test_KErrNone(r);
        temp.Close();
        ReadDirDisk(dirBuf);
        WriteToFatBuf(gClusterDir2_Full,gClusterDir2_SD3E,fatBuf.Ptr());
        WriteToFatBuf(gClusterDir2_SD3E,gClusterDir2_SD23E,fatBuf.Ptr());
        WriteToFatBuf(gClusterDir2_SD23E,gEndOfChain,fatBuf.Ptr());
        if (last > 0)
            WriteToFatBuf(last,0,fatBuf.Ptr());
        TEntryInfo partial5(ClusterEntryToBytes(gClusterDir2_Full,gEntriesPerCluster-2),19);
        CreatePartialEntry(partial5,7,EFalse);
        res=TestPartialEntry(partial5);
        test(res);
        r=TheFs.Delete(longFile);
        test_Value(r, r==KErrNone || r==KErrNotFound);
        r=TheFs.Delete(_L("\\temp"));
        test_Value(r, r==KErrNone || r==KErrNotFound);
        }
    ReadDirDisk(dirBuf);

    test.End();
    }

LOCAL_C void DoMatchingEntries()
//
// Tests that scandrive detects/corrects entries with the same start cluster
// Copies entry to new location - replicates start cluster
//
    {
    test.Next(_L("Check matching entries"));
    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);

    // first entry in \scndrv\almostfull\ + root dir
    test.Start(_L("matching entries in subdir + root dir"));
    TEntryInfo from1(ClusterEntryToBytes(gClusterDir2_AFull,2),6);
    TEntryInfo to1(ClusterEntryToBytes(gClusterRootDir,2),6);
    CreateMatchingEntry(to1,from1,EFalse);
    TBool res=TestMatchingEntry(to1);
    test(res);

    // matching entries between 2 subdirs, one which has a full cluster
    // first entry in \scndrv\dir2\full\ + end of \scndrv\dir2\almostfull
    test.Next(_L("matching entries between 2 subdirs"));
    TEntryInfo from2(ClusterEntryToBytes(gClusterDir2_Full,2),2);
    TEntryInfo to2(ClusterEntryToBytes(gClusterDir2_AFull,14),2);
    CreateMatchingEntry(to2,from2,EFalse);
    res=TestMatchingEntry(to2);
    test(res);

    // matching entries between two subdirs - one with end of dir marker next
    // \scndrv\dir2\somedirwith3entries to \scndrv
    test.Next(_L("matching entries between two subdirs"));
    TEntryInfo from3(ClusterEntryToBytes(gClusterDir2,4),3);
    TEntryInfo to3(ClusterEntryToBytes(gClusterScnDrv,6),3);
    CreateMatchingEntry(to3,from3,ETrue);
    res=TestMatchingEntry(to3);
    test(res);

    // matching entries in same subdir, one in new cluster - irrelevant if matching names
    // 1st and last entries in \scndrv\dir2\full
    test.Next(_L("matching entries in same subdir"));
    // delete entries to allow contiguous clusters in \scndrv\dir2\full directory
    TInt r=TheFs.RmDir(_L("\\scndrv\\dir2\\somedirwith3entries\\"));
    test_KErrNone(r);
    r=TheFs.RmDir(_L("\\scndrv\\dir2\\somedir2with3entries\\"));
    test_KErrNone(r);
    // ensure directory is expanded
    RFile temp;
    r=temp.Create(TheFs,_L("\\scndrv\\dir2\\full\\temp"),EFileShareAny);
    test_KErrNone(r);
    temp.Close();
    r=TheFs.Delete(_L("\\scndrv\\dir2\\full\\temp"));
    test_KErrNone(r);
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);
    TEntryInfo from4(ClusterEntryToBytes(gClusterDir2_Full,4),2);
    TEntryInfo to4(ClusterEntryToBytes(gClusterDir2_Full+1,0),2);
    CreateMatchingEntry(to4,from4,ETrue);
    res=TestMatchingEntry(to4);
    test(res);

    // \scndrv\dir1\very long name to \\scndrv\dir2\full
    test.Next(_L("matching entries in diff dirs + new cluster"));
    // delete last entry in directory
    r=TheFs.Delete(LastInFull);
    test_KErrNone(r);
    TFileName veryLongName=_L("\\scndrv\\dir2\\full\\");
    MakeVeryLongName(veryLongName);
    r=temp.Create(TheFs,veryLongName,EFileShareAny);
    test_KErrNone(r);
    temp.Close();
    r=TheFs.Delete(veryLongName);
    test_KErrNone(r);
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);
    TEntryInfo from5(ClusterEntryToBytes(gClusterDir1,2),19);
    TEntryInfo to5(ClusterEntryToBytes(gClusterDir2_Full,gEntriesPerCluster-2),19);
    CreateMatchingEntry(to5,from5,EFalse);
    res=TestMatchingEntry(to5);
    test(res);

    test.End();
    }


LOCAL_C void DoMaxDepth()
//
// Test directory structure with max possible depth
//
    {
    test.Next(_L("Check max directory depth"));
    test.Start(_L("Using single character names"));
    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();
    // Create dir structure
    TFileName dir1=_L("\\");
    TFileName dir2;
    CreateMaxDepthDir(dir1,dir2);
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);
    // run scandisk and compare
    TInt r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    TBool res=IsSameAsDrive(fatBuf,dirBuf);
    test(res);
    // Create a entry with matching start cluster and check fixed up
    TEntryInfo from(ClusterEntryToBytes(gClusterDir2_AFull,2),6);
    TEntryInfo to(ClusterEntryToBytes(gClusterEndMaxDepth,2),6);
    CreateMatchingEntry(to,from,ETrue);
    res=TestMatchingEntry(to);
    test(res);
    // DeleteMaxDepthStructure
    DeleteMaxDepthDir(dir1,dir2);
    test.End();
    }

LOCAL_C void DoRootDir()
//
// Check that a full root directory is searched OK
//
    {
    test.Next(_L("Check a full root directory"));
    FillUpRootDir();
    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);

    TInt r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);

    TBool res=IsSameAsDrive(fatBuf,dirBuf);
    test(res);
    UnFillUpRootDir();
    }

LOCAL_C void TestNonVfatNames(const TPtrC& aDirName, TInt aDirCluster, TInt aEntry=2)
//
// Check that files without 'long' entries are kept intact.  Creates files with
// a DOS type name, and for each one created except the last deletes the VFAT
// entry by copying the DOS entry over it and writing end of directory.  This
// leaves a VFAT entry at the end of the directory, except when there is only
// room for one file.
//
// The layout, for 1 sector per cluster, is thus like:
//    0 .
//    1 ..
//    2 TEMPFILE.000
//    3 TEMPFILE.001
//      ...
//   14 tempfile.012 VFAT
//   15 TEMPFILE.012
//
// or for an almost full directory
//
//    0 .
//    1 ..
//      whatever...
//   14 TEMPFILE.000
//   15 end of directory
//
    {
    test.Printf(_L("Test cluster %2d, aEntry %d: %S\n"), aDirCluster, aEntry, &aDirName);
    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();
    TInt cluster = aDirCluster;

    TInt maxEntry = gEntriesPerCluster;
    if (aDirName.Compare(_L("\\")) == KErrNone)
        maxEntry = Min(gRootDirEntries, gEntriesPerCluster);

    TInt entry = aEntry;
    TInt r = KErrNone;
    TInt i;

    while (entry > gEntriesPerCluster)
        {
        entry -= gEntriesPerCluster;
        cluster++;
        }

    TInt nFiles = maxEntry - entry - 1;
    TInt startEntry = entry;

    test.Printf(_L("cluster %d, entry %d maxEntry %d, nFiles %d\n"), cluster, entry, maxEntry, nFiles);

    TBuf8<256> buf;
    buf.Fill('*', 256);

    // Set up files, ignoring used slots
    TInt filesThisTime = nFiles;
    TInt totalFilesCreated = 0;
    FOREVER
        {
        //
        // Create a number of VFat entries
        //
        //  - We create as many as we can fit in the cluster in one go.
        //    This is faster than creating a single entry then copying, as writing the
        //    entries one at a time using RRawDisk causes a remount of the file system,
        //    which can take a very long time on a large disk.
        //
        filesThisTime = (nFiles - totalFilesCreated) >> 1;
        if(filesThisTime == 0)
            {
            if(nFiles == totalFilesCreated)
                {
                test.Printf(_L("Created all Non-VFAT entries\n"));
                break;
                }

            //...creating the final entry
            filesThisTime = 1;
            }

        for (i = 0; i < filesThisTime; i++)
            {
            TFileName name(aDirName);
            name.Append(_L("tempfile."));
            name.AppendNumFixedWidth(i+totalFilesCreated, EHex, 3);
            RFile f;
            r = f.Create(TheFs, name, EFileShareAny);
            test_KErrNone(r);
            r = f.Write(buf);
            test_KErrNone(r);
            f.Close();
            }

        //
        // Move DOS FAT entries up using RRawDisk, deleting the original VFAT entries
        //
        ReadDirDisk(dirBuf, cluster);
        TInt dosEntry = entry + 1;
        for (i = 0; i < filesThisTime; i++)
            {
            // Copy VFAT to Non-VFAT entries
            if (entry+2 < maxEntry || nFiles < 2)
                {
                TInt posVFAT = ClusterEntryToBytes(cluster, entry);
                TInt posEOD = ClusterEntryToBytes(cluster, entry+1);
                TInt posDOS = ClusterEntryToBytes(cluster, dosEntry);

                WriteCopyDir(posDOS, posVFAT);  // Copy the DOS entry
                WriteDelete(posDOS,2);          // Delete the original entry
                WriteEndOfDir(posEOD);          // Write End Of Directory

                entry += 1;
                dosEntry += 2;
                }
            else
                {
                // last entry has VFAT intact, to fill cluster
                entry += 2;
                }

            }

        WriteDirEntryToDisk(dirBuf, cluster);
        totalFilesCreated += filesThisTime;
        test.Printf(_L("   created %d entries\n"), totalFilesCreated);
        }

    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);

    DumpData(NULL, aDirCluster, cluster+1);

    test.Printf(_L("Running ScanDrive\n"), filesThisTime);
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);

    TBool res=IsSameAsDrive(fatBuf,dirBuf);
    test(res);

    test.Printf(_L("Deleting %d files\n"), nFiles);
    for (i = 0; i < nFiles; i++)
        {
        TFileName name(aDirName);
        name.Append(_L("tempfile."));
        name.AppendNumFixedWidth(i, EHex, 3);
        r = TheFs.Delete(name);
        test_KErrNone(r);
        }

    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);
    WriteEndOfDir(ClusterEntryToBytes(cluster, startEntry));
    WriteDirEntryToDisk(dirBuf);

    test.Printf(_L("Running ScanDrive\n"), filesThisTime);
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    res=IsSameAsDrive(fatBuf,dirBuf);
    test(res);
    }

LOCAL_C void DoNonVfatNames()
//
// Check that files without 'long' entries are kept intact
//
    {
    test.Next(_L("Check non-VFAT file names"));
    TestNonVfatNames(_L("\\"), gClusterRootDir, 2);
    TestNonVfatNames(_L("\\scndrv\\dir1\\"), gClusterDir1, 2+19);
    TestNonVfatNames(_L("\\scndrv\\dir2\\somedirwith3entries\\"), gClusterDir2_SD3E, 2);
    TestNonVfatNames(_L("\\scndrv\\dir2\\almostfull\\"), gClusterDir2_AFull, 14);
    }


LOCAL_C void DoTests()
    {

    Format();
    DoReadBootSector();
    DumpBootSector();
    InitialiseBuffers();
    CreateRootDir();
    CreateDirectoryStructure();
    TPtr8 fatBuf=FatBufPtr->Des();
    TPtr8 dirBuf=DirBufPtr->Des();
    ReadDirDisk(dirBuf);
    ReadFatDisk(fatBuf);
    DumpFat();
    DumpData(NULL, DirBufPtr->Ptr());

    DoNonVfatNames();
    DoRootDir();
    DoMaxDepth();
    DoMatchingEntries();
    DoPartEntries();
    DoLostClusters();
    DoHangingClusters();
    DoHangingAndLostClusters();
    TestMountAndScan();
    TestConsecutiveMountAndScans();
    DeleteDirectoryStructure();
    DeleteRootDir();
    TestExtendedChars();

    DumpBootSector();
    DumpFat();
    DumpData(NULL, 0, 200);

    delete FatDiskPtr;
    delete DirDiskPtr;
    delete FatBufPtr;
    delete DirBufPtr;
    }


void CallTestsL()
    {
    TInt r;
    r = TheFs.CharToDrive(gSessionPath[0], gDriveNumber);
    test_KErrNone(r);


    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

    //-- print drive information
    PrintDrvInfo(TheFs, gDriveNumber);

    if (!Is_Fat(TheFs, gDriveNumber))
        {
        test.Printf(_L("CallTestsL: Skipped: test requires FAT filesystem\n"));
        return;
        }

    // check this is not the internal ram drive
    TVolumeInfo v;
    r=TheFs.Volume(v);
    test_KErrNone(r);
    if(v.iDrive.iMediaAtt&KMediaAttVariableSize)
        {
        test.Printf(_L("Error: Internal ram drive not tested\n"));
        return;
        }

    r=TheFs.SetSessionPath(gSessionPath);
    test_KErrNone(r);

    DoTests();

    return;
    }






