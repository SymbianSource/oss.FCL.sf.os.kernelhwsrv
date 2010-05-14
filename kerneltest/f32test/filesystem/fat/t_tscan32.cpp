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
// f32test\filesystem\fat\t_tscan32.cpp
// 

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>

#include "t_server.h"

#include "fat_utils.h"
using namespace Fat_Test_Utils;

/*
Series of tests to check that the combination of a ruggedised fat file
system and scandisk prevents the file system from becoming corrupt in the
event of a power failure. CheckDisk is used to test that directory
structure is not corrupt. This test is only suitable with a drive that uses
the fat file system but not the internal ram drive (due to the indirection)
table. Only works with debug builds due to RFs::ControlIo only debug function
*/

GLDEF_D TFileName StartupExeName=_L(""); // initialised at run time

#ifdef _DEBUG
GLREF_D RTest test;
GLDEF_D TInt TheFunctionNumber;	// Indicates which test to run
GLDEF_D TInt TheOpNumber;		// Indicates which file operation to be tested
GLDEF_D TInt TheFailCount;		
GLDEF_D TBool IsReset;
GLDEF_D TFileName TestExeName=_L("?:\\T_SCANDR.EXE"); //Renaming it to fit in one root dir entry.
GLDEF_D TFileName LogFileName=_L("?:\\T_SCANDR.LOG"); //Renaming it to fit in one root dir entry.

const TInt KControlIoWriteFailOn=0;		// commands to pass into RFs::ControlIo
const TInt KControlIoWriteFailOff=1;
const TInt KMaxFatEntries  = 2048;
const TInt KDirAttrReadOnly  = 0x01;
const TInt KDirAttrHidden    = 0x02;
const TInt KDirAttrSystem    = 0x04;
const TInt KDirAttrVolumeId  = 0x08;
const TInt KDirAttrDirectory = 0x10;
const TInt KDirAttrArchive   = 0x20;
const TInt KDirAttrLongName  = KDirAttrReadOnly | KDirAttrHidden | KDirAttrSystem | KDirAttrVolumeId;
const TInt KDirAttrLongMask  = KDirAttrLongName | KDirAttrDirectory | KDirAttrArchive;
const TInt KDirLastLongEntry = 0x40;

GLDEF_D TInt WriteFailValue;	// Indicates what error should return from a write failure
								// Value assigned in t_scn32dr2 and t_scn32dr3
LOCAL_C TFatBootSector BootSector;	
LOCAL_D RRawDisk TheRawDisk;

static TFatType gDiskType = EInvalid;

LOCAL_D TInt gTotalSectors;
LOCAL_D TInt gBytesPerCluster;
LOCAL_D TInt gRootDirSectors;
LOCAL_D TInt gRootDirEntries;
LOCAL_D TInt gRootDirStart;
LOCAL_D TInt gRootSector;
LOCAL_D TInt gFatStartBytes;
LOCAL_D TInt gFatTestEntries;
LOCAL_D TInt gFatSizeSectors;
LOCAL_D TInt gFirstDataSector;
LOCAL_D TInt gDataStartBytes;
LOCAL_D TInt gClusterCount;

LOCAL_D HBufC8* gFatBuf  = NULL;
LOCAL_D TInt    gFatAddr = -1;

enum TFatChain
	{
	EChainStd,			// Cluster chain grows contiguously
	EChainAlternate,	// Cluster chain grows forward but not contiguously
	EChainBackwards,	// Cluster chain first goes backwards(up to 3.5kb for fat16 file) and then forwards
	EChainForwards		// Cluster chain first goes forward (upto 3.5kb for fat16 file) and then backwards
	};

LOCAL_C TBool IsInternalRam()
//
// Returns true if the selected drive is variable size (i.e. RAM drive)
//
	{
	TVolumeInfo v;
	TInt r=TheFs.Volume(v,gSessionPath[0]-'A');
	test_KErrNone(r);
	return(v.iDrive.iMediaAtt&KMediaAttVariableSize);
	}

LOCAL_C void WriteLogFile()
//
// Writes state of test to end of LogFileName
//
	{
	test.Printf(_L("Writelogfile()\n"));
	RFile log;
	TInt r=log.Open(TheFs,LogFileName,EFileShareExclusive|EFileWrite);
	if(r!=KErrNone)
		test.Printf(_L("error=%d\n"),r);
	test_KErrNone(r);
	TInt size;
	r=log.Size(size);
	test_KErrNone(r);
	TBuf8<16> buf;
	buf.SetLength(4);
	buf[0]=(TUint8)TheFunctionNumber;
	buf[1]=(TUint8)TheOpNumber;
	buf[2]=(TUint8)TheFailCount;
	buf[3]='\n';
	r=log.Write(size,buf,buf.Length());
	test_KErrNone(r);
	test.Printf(_L("Written func=%d,op=%d,fail=%d\n"),TheFunctionNumber,TheOpNumber,TheFailCount);
	log.Close();
	}

LOCAL_C TInt SetWriteFailOn(TInt aFailCount)
//
// Sets write to metadata to fail on aFailCount with WriteFailValue
//
	{
	TInt16 args[2];
	TPtr8 des((TUint8*)args,4,4);
	args[0]=(TUint16)aFailCount;
	args[1]=(TUint16)WriteFailValue;
	TInt r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOn,des);
	return(r);
	}

GLDEF_C void ReadLogFile()
//
// Reads state of test from end of LogFileName and sets global variables
//
	{
	test.Next(_L("ReadLogFile"));
	RFile log;
	TInt r=log.Open(TheFs,LogFileName,EFileShareExclusive);
	if(r!=KErrNone)
		test.Printf(_L("error in ReadLogFile()=%d\n"),r);
	test_KErrNone(r);
	
	TInt fileSize;
	r=log.Size(fileSize);
	if(fileSize==0)
		{
		TheFunctionNumber=0;
		TheOpNumber=0;
		TheFailCount=0;
		}
	else
		{
		TBuf8<4> buf;
		r=log.Read(fileSize-4,buf,4);
		TheFunctionNumber=buf[0];
		TheOpNumber=buf[1];
		TheFailCount=buf[2];
		}
	log.Close();
	test.Printf(_L("func=%d,op=%d,fail=%d\n"),TheFunctionNumber,TheOpNumber,TheFailCount);
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
	TInt sector = (aCluster - 2) * gBytesPerCluster + gFirstDataSector * BootSector.BytesPerSector();
	return sector;
	}

/**
    Fill media with zeroes from aStartPos to aEndPos
*/
static void DoZeroFillMedia(TInt64 aStartPos, TInt64 aEndPos, RRawDisk& aWriter)
{
	test(aStartPos >=0 && aEndPos >=0 && aStartPos < aEndPos);
    
    if(aStartPos == aEndPos)
        return;

    RBuf8 buf;
    TInt  nRes;  

    const TUint32 KBufSz=65536*2; //-- buffer with zeroes
    
    nRes = buf.CreateMax(KBufSz);
    test_KErrNone(nRes);

    buf.FillZ();

    TUint32 rem = (TUint32)(aEndPos - aStartPos);
    while(rem)
    {
        const TUint32 bytesToWrite=Min(rem, KBufSz);
    
        TPtrC8 ptr(buf.Ptr(), bytesToWrite);
        nRes = aWriter.Write(aStartPos, ptr);
        test_Value(nRes, nRes == KErrNone || nRes == KErrDiskFull);

        aStartPos+=bytesToWrite;
        rem-=bytesToWrite;
    }


    buf.Close();
}

//
// Clear the disk data area to a known value which won't be confused with
// directory entries etc.
//
LOCAL_C void ClearDiskData()
	{

	TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
	test_KErrNone(r);

	TUint32 startPos = gDataStartBytes;
	if (gDiskType == EFat32)
		startPos += gBytesPerCluster;

    const TUint32 endPos = startPos + gFatTestEntries*gBytesPerCluster;

    test.Printf(_L("ClearDiskData() from pos:%u to pos:%u\n"), startPos, endPos);

    DoZeroFillMedia(startPos, endPos, TheRawDisk);

	TheRawDisk.Close();
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
	gBytesPerCluster   = BootSector.BytesPerSector() * BootSector.SectorsPerCluster();
	gRootDirSectors    = ((BootSector.RootDirEntries() * KSizeOfFatDirEntry + BootSector.BytesPerSector() - 1) / BootSector.BytesPerSector());
	gFatStartBytes     = BootSector.ReservedSectors() * BootSector.BytesPerSector();
	gFatSizeSectors    = (BootSector.FatSectors() ? BootSector.FatSectors() : BootSector.FatSectors32());
	gRootSector        = BootSector.ReservedSectors() + BootSector.NumberOfFats() * gFatSizeSectors;
	gRootDirStart      = gRootSector * BootSector.BytesPerSector();
	gFirstDataSector   = gRootSector + gRootDirSectors;

	gFatTestEntries = MaxClusters();
	if (gFatTestEntries > KMaxFatEntries)
		gFatTestEntries = KMaxFatEntries;

	if (BootSector.RootDirEntries() == 0)
		{
		test.Printf(_L("**** Is Fat32\n"));
		gDiskType = EFat32;
		gRootDirEntries = BootSector.RootDirEntries();
		}
	else if (BootSector.FatType() == EFat16)
		{
		test.Printf(_L("**** Is Fat16\n"));
		gDiskType = EFat16;
		gRootDirEntries = BootSector.RootDirEntries();
		}
	else
		{
		test.Printf(_L("**** Is Fat12\n"));
		gDiskType = EFat12;
		gRootDirEntries = gBytesPerCluster * 1 / KSizeOfFatDirEntry;
		}
	gTotalSectors   = (BootSector.TotalSectors() ? BootSector.TotalSectors() : BootSector.HugeSectors());
	gClusterCount   = (gTotalSectors - gFirstDataSector) / BootSector.SectorsPerCluster();
	gDataStartBytes = gFirstDataSector * BootSector.BytesPerSector();
	}

GLDEF_C TUint32 GetFatEntry(TUint32 aIndex, const TUint8* aFat=NULL)
//
// Read a single FAT entry from disk or FAT copy and return it
//
	{


	if (!gFatBuf)
		{
		gFatBuf=HBufC8::New(gBytesPerCluster);
		test_NotNull(gFatBuf);
		gFatAddr = -1;
		}

	const TUint8* ptr;

	if (aFat)
		{
		ptr = (TUint8*)aFat + PosInBytes(aIndex);
		}
	else
		{
		TInt pos = PosInBytes(aIndex) + gFatStartBytes;
		if (gFatAddr < 0 || pos < gFatAddr || pos >= gFatAddr + gBytesPerCluster)
			{
			TPtr8 ptr=gFatBuf->Des();
			TInt r=TheRawDisk.Open(TheFs,gSessionPath[0]-'A');
			test_KErrNone(r);
			r=TheRawDisk.Read(pos, ptr);
			test_KErrNone(r);
			TheRawDisk.Close();
			gFatAddr = pos;
			}
		ptr = gFatBuf->Ptr() + pos-gFatAddr;
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

	//New for FAT32

	if(BootSector.RootDirEntries() == 0)	//indicates we have FAT32 volume
		{
		RDebug::Print(_L("FatSectors32      = %8d"), BootSector.FatSectors32());
		RDebug::Print(_L("FATFlags          = %8d"), BootSector.FATFlags());
		RDebug::Print(_L("VersionNumber     = %8d"), BootSector.VersionNumber());
		RDebug::Print(_L("RootClusterNum    = %8d (0x%08X)"), BootSector.RootClusterNum(), gRootDirStart);
		RDebug::Print(_L("FSInfoSectorNum   = %8d (0x%08X)"), BootSector.FSInfoSectorNum(), BootSector.FSInfoSectorNum() * BootSector.BytesPerSector());
		RDebug::Print(_L("BkBootRecSector   = %8d (0x%08X)"), BootSector.BkBootRecSector(), BootSector.BkBootRecSector() * BootSector.BytesPerSector());
		}
	TInt fatEntries = gFatSizeSectors*BootSector.BytesPerSector();
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
	RDebug::Print(_L("ClusterCount      = %8d (%d bytes)"), gClusterCount, gClusterCount*gBytesPerCluster);
	RDebug::Print(_L("FatEntries        = %8d (%d sectors)"), fatEntries, gFatSizeSectors);
	RDebug::Print(_L("RootSector        = %8d (0x%08X)"), gRootSector, gRootDirStart);
	RDebug::Print(_L("FirstDataSector   = %8d (0x%08X)"), gFirstDataSector, gDataStartBytes);
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
	LOCAL_D TBuf<6> str(_L("------"));
	LOCAL_D char*   atr = "RHSVDA";
	for (TInt i = 0; i < 6; i++)
		if ((aAttrib >> i) & 1)
			str[i] = atr[i];
	return &str;
	}

GLDEF_C TBool IsValidDirChar(TUint8 aChar, TUint8 aMin=0x20)
//
// Test whether a character is valid as part of a short filename, aMin is to
// distinguish between first character (which can't be space) and later ones
// which can include space but nothing less.  Note that E5 is a valid character
// in any position, even though it means 'erased' in the first character.
//
	{
	const TUint8* inval = (TUint8*)"\x22\x2A\x2B\x2C\x2E\x2F\x3A\x3B\x3C\x3D\x3E\x3F\x5B\x5C\x5D\x7C";
	if (aChar < aMin)
		return EFalse;
	for (const TUint8* p = inval; *p; p++)
		if (aChar == *p)
			return EFalse;
	return ETrue;
	}

GLDEF_C TBool IsValidDirEntry(TFatDirEntry* aDir)
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
		// RDebug::Print(_L("%5d: ERASED"), aNum);
		}
	else if (d->IsEndOfDirectory())
		return EFalse;
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
		return EFalse;
	else
		{
		TBuf<11> name;
		name.Copy(d->Name());
		RDebug::Print(_L("%5d: '%S'  %S  cluster %d"),
					  aNum, &MakePrintable(name), DirAttributes(d->Attributes()), d->StartCluster());
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

GLDEF_C void DumpData(const TUint8* aFat, TInt aStart, TInt aEnd)
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

GLDEF_C void DumpHex(const TUint8* aData, TInt aLen)
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
		RDebug::Print(_L("%04X: %S"), off, &buf);
		}
	}

static void QuickFormat()
    {
        /*
        TFatFormatParam fmt;
        fmt.iFatType = EFat32;
        fmt.iSecPerCluster =1;
        FormatFatDrive(TheFs, CurrentDrive(), ETrue, &fmt);
        */
        FormatFatDrive(TheFs, CurrentDrive(), ETrue);
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

LOCAL_C void MakeEntryName(TFileName& aName,TInt aLength)
//
// Appends aLength characters to aName
//
	{
	for(TInt i=0;i<aLength;++i)
		{
		TInt c='A'+i%26;
		aName.Append(c);
		}
	}

LOCAL_C void FillUpRootDir(TInt aFree=0)
//
// Fill up root directory
//
	{
	TInt maxRootEntries = gRootDirEntries -aFree;
	TFileName dir=_L("\\??\\");
	TInt count=0;
	TInt entriesSoFar;
	if(IsReset)
		entriesSoFar=2+2+2+2; // \\t_scn32dr3.exe + \\sys + \\t_scn32dr3.log + \\f32-tst
	else
		entriesSoFar=0;
	TInt r;
	while(entriesSoFar<maxRootEntries)
		{
		dir[1]=TUint16(count/26+'a');
		dir[2]=TUint16(count%26+'a');
		r=TheFs.MkDir(dir);
		test_KErrNone(r);
		entriesSoFar+=2;
		++count;
		}
	}

LOCAL_C void UnFillUpRootDir(TInt aFree=0)
//
// Reverse changes from FillUpRootDir()
//
	{
	TFileName dir=_L("\\??\\");
	TInt entriesSoFar=gRootDirEntries -aFree;
	TInt count=0;
	TInt r;
	TInt existing;
	if(IsReset)
		existing=2+2+2+2; // \\t_scn32dr3.exe + \\sys + \\t_scn32dr3.log + \\f32-tst
	else
		existing=0;
	while(entriesSoFar>existing)
		{
		dir[1]=TUint16(count/26+'a');
		dir[2]=TUint16(count%26+'a');
		r=TheFs.RmDir(dir);
		test_KErrNone(r);
		entriesSoFar-=2;
		++count;
		}
	}

void InitialiseWriteBuffer(TDes8& buf)
//
//
//
	{
	for(TInt i=0;i<buf.Length();++i)
		buf[i]=(TUint8)('a'+i%26);
	}

LOCAL_C TBool EntryExists(const TDesC& aName)	
//
// Returns ETrue if aName is found
//
	{
	TEntry entry;
	TInt r=TheFs.Entry(aName,entry);
	test_Value(r, r==KErrNone||r==KErrNotFound);
	return(r==KErrNone?(TBool)ETrue:(TBool)EFalse);
	}

LOCAL_C TInt EntriesPerFatSector()
//
// Returns number of entries in one fat table sector
//
	{
	switch (gDiskType)
		{
		case EFat32:
			return(BootSector.BytesPerSector()/4);
		case EFat16:
			return(BootSector.BytesPerSector()/2);
		case EFat12:
			return(BootSector.BytesPerSector()*2/3);
		default:
			test(0);
		}
	return -1;
	}

LOCAL_C TBool OneEntryExists(const TDesC& aOldName,const TDesC& aNewName)
//
// Returns ETrue if only one of two entries exists
//
	{
	TBool oldExists=EntryExists(aOldName);
	TBool newExists=EntryExists(aNewName);
	return((!oldExists&&newExists)||(oldExists&&!newExists));
	}

LOCAL_C void GetEntryDetails(const TDesC& aName,TEntry& aEntry)
//
// returns entry details for the entry with aName
//
	{
	TInt r=TheFs.Entry(aName,aEntry);
	test_KErrNone(r);
	}

LOCAL_C TBool IsSameEntryDetails(TEntry aOldEntry,TEntry aNewEntry)
//
// 
//
	{
	return(aOldEntry.iAtt==aNewEntry.iAtt&&aOldEntry.iSize==aNewEntry.iSize&&aOldEntry.iModified==aNewEntry.iModified);
	}

LOCAL_C void CreateAlternate(const TDesC& aNameOne,const TDesC& aNameTwo)
//
// Creates altenate entries which take up one sector of fat table.
// By subsequently deleting  one of these entries a new entry can be made 
// with cluster chain that is not contiguous.
//
	{
	TInt entries=EntriesPerFatSector();
	RFile file1,file2;
	TInt size1,size2;
	size1=size2=0;
	TInt r=file1.Create(TheFs,aNameOne,EFileShareAny);
	test_KErrNone(r);
	r=file2.Create(TheFs,aNameTwo,EFileShareAny);
	test_KErrNone(r);
	// one entry for file1 for every 40 entries for file2
	// if file 1 subseqently deleted then 7 entries available
	// in that fat sector - ~3.5kb file size - for fat16
	TInt ratio=40;
	TBool first=ETrue;
	while(entries>0)
		{
		if(first)
			{
			size1+=gBytesPerCluster;
			r=file1.SetSize(size1);
			test_KErrNone(r);
			first=EFalse;
			--entries;
			}
		else
			{
			size2+=gBytesPerCluster*ratio;
			r=file2.SetSize(size2);
			test_KErrNone(r);
			first=ETrue;
			entries-=ratio;
			}
		}
	file1.Close();
	file2.Close();
	}

LOCAL_C TInt ThrottleDirEntries(TInt aDirEntries)
	{
	// throttle the number of entries needed, since for large cluster
	// sizes, this can take forever (eg 2GB card -> a cluster size of 32K
	// -> 1024 entries per cluster
	const TInt KMaxDirEntries = 2048;
	if (aDirEntries > KMaxDirEntries)
		{
		RDebug::Print(_L("Reducing directory entries from %d to %d"), 
			aDirEntries, KMaxDirEntries);
		aDirEntries = KMaxDirEntries;
		}
	return aDirEntries;
	}


LOCAL_C void CleanDirectory(const TDesC& aName,TInt aClusters)
//
// Removes entries in the directory
//
	{
	if (aClusters < 1)
		return;
	TInt entriesPerCluster=gBytesPerCluster/32;
	TInt entriesNeeded = entriesPerCluster * aClusters;

	entriesNeeded = ThrottleDirEntries(entriesNeeded);

	TInt maxFileNameLen = 250 - aName.Length();
	TInt nameEntries = 1 + (maxFileNameLen+12) / 13;
	TInt namesNeeded = (entriesNeeded + nameEntries-1) / nameEntries;
	TInt entry = 0;
	for(TInt i = 0; i < namesNeeded; ++i)
		{
		if (entriesNeeded - entry < nameEntries)
			maxFileNameLen = (entriesNeeded - entry - 1) * 13;
		TFileName fn;
		fn.AppendNum(entry);
		fn.Append('_');
		while (fn.Length() < maxFileNameLen)
			fn.Append('0');
		TFileName fullName(aName);
		fullName.Append(fn);
		TInt r = TheFs.Delete(fullName);
		test_KErrNone(r);
		entry += 1 + (fn.Length() + 12) / 13;
		}
	RDebug::Print(_L("CleanDirectory(%S, %d)"), &aName, aClusters);
	}

LOCAL_C void ExpandDirectory(const TDesC& aName,TInt aClusters)
//
// Expands the directory by aClusters
//
	{
	if (aClusters < 1)
		return;
	TInt entriesPerCluster=gBytesPerCluster/32;
	TInt entriesNeeded = entriesPerCluster * aClusters;

	entriesNeeded = ThrottleDirEntries(entriesNeeded);

	TInt maxFileNameLen = 250 - aName.Length();
	TInt nameEntries = 1 + (maxFileNameLen+12) / 13;
	TInt namesNeeded = (entriesNeeded + nameEntries-1) / nameEntries;
	TInt entry = 0;
	for(TInt i = 0; i < namesNeeded; ++i)
		{
		if (entriesNeeded - entry < nameEntries)
			maxFileNameLen = (entriesNeeded - entry - 1) * 13;
		TFileName fn;
		fn.AppendNum(entry);
		fn.Append('_');
		while (fn.Length() < maxFileNameLen)
			fn.Append('0');
		TFileName fullName(aName);
		fullName.Append(fn);
		RFile file;
		TInt r = file.Create(TheFs,fullName,EFileShareAny);
		test_KErrNone(r);
		file.Close();
		entry += 1 + (fn.Length() + 12) / 13;
		}
	// to leave a directory expanded by aClusters but with no additional entries
	RDebug::Print(_L("ExpandDirectory(%S, %d)"), &aName, aClusters);
	CleanDirectory(aName,aClusters);
	}

LOCAL_C TInt DeleteAlternateEntry(const TDesC& aName,TBool aIsDir)
//
// Deletes entry aName and corresponding entries created for EChainAlternate
//
	{
	TInt r=TheFs.Delete(_L("\\fat\\file2"));
	test_Value(r, r==KErrNone||r==KErrNotFound);
	if(aIsDir)
		return(TheFs.RmDir(aName));
	else
		return(TheFs.Delete(aName));
	}

LOCAL_C TInt CreateAlternateEntry(const TDesC& aName,TBool aIsDir,TInt aSize)
//
// Creates entry with aName where cluster chain grows forward but not contiguously.
// Assumes that no holes in fat clusters.
//
	{
	TInt r=DeleteAlternateEntry(aName,aIsDir);
	test_Value(r, r==KErrNone||r==KErrNotFound);
	RFile file;
	if(aIsDir)
		{
		r=TheFs.MkDir(aName);
		if(r!=KErrNone)
			return(r);
		}
	else
		{
		r=file.Create(TheFs,aName,EFileShareAny);
		if(r!=KErrNone)
			return(r);
		r=file.SetSize(1); //ensure file allocated a start cluster
		test_KErrNone(r);
		}
	CreateAlternate(_L("\\fat\\file1"),_L("\\fat\\file2"));
	r=TheFs.Delete(_L("\\fat\\file1"));
	test_KErrNone(r);
	if(aIsDir)
		ExpandDirectory(aName,aSize);
	else
		{
		r=file.SetSize(aSize);
		test_KErrNone(r);
		file.Close();
		}
	return(KErrNone);
	}

LOCAL_C TInt DeleteForwardEntry(const TDesC& aName,TBool aIsDir)
//
// Deletes entry with aName and corresponding entries created for EChainForward
//
	{
	TInt r=TheFs.Delete(_L("\\fat\\file2"));
	test_Value(r, r==KErrNone||r==KErrNotFound);
	r=TheFs.Delete(_L("\\fat\\file4"));
	test_Value(r, r==KErrNone||r==KErrNotFound);
	r=TheFs.Delete(_L("\\fat\\file5"));
	test_Value(r, r==KErrNone||r==KErrNotFound);
	if(aIsDir)
		r=TheFs.RmDir(aName);
	else
		r=TheFs.Delete(aName);
	return r;
	}
	
LOCAL_C TInt CreateForwardEntry(const TDesC& aName,TBool aIsDir,TInt aSize)
//
// Creates an entry whose cluster chain first goes forward (upto 3.5kb for fat16 file)
// and then backwards
//	
	{
	TInt r=DeleteForwardEntry(aName,aIsDir);
	test_Value(r, r==KErrNone||r==KErrNotFound);
	RFile file1,file2,entry;
	r=file1.Create(TheFs,_L("\\fat\\file1"),EFileShareAny);
	test_KErrNone(r);
	r=file1.SetSize(EntriesPerFatSector()*gBytesPerCluster);
	test_KErrNone(r);
	r=file2.Create(TheFs,_L("\\fat\\file2"),EFileShareAny);
	test_KErrNone(r);
	r=file2.SetSize(EntriesPerFatSector()*gBytesPerCluster);
	test_KErrNone(r);
	if(aIsDir)
		{
		r=TheFs.MkDir(aName);
		if(r!=KErrNone)
			return(r);
		}
	else
		{
		r=entry.Create(TheFs,aName,EFileShareAny);
		if(r!=KErrNone)
			return(r);
		r=entry.SetSize(1);	// ensure entry has start cluster allocated
		test_KErrNone(r);
		}
	CreateAlternate(_L("\\fat\\file3"),_L("\\fat\\file4"));
	RFile file5;
	r=file5.Create(TheFs,_L("\\fat\\file5"),EFileShareAny);
	test_KErrNone(r);
	r=file5.SetSize(EntriesPerFatSector()*gBytesPerCluster*2);
	test_KErrNone(r);
	file1.Close();
	file2.Close();
	file5.Close();
	r=TheFs.Delete(_L("\\fat\\file1"));
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\fat\\file3"));
	test_KErrNone(r);
	if(aIsDir)
		ExpandDirectory(aName,aSize);
	else
		{
		r=entry.SetSize(aSize);
		test_KErrNone(r);
		entry.Close();
		}
	return(KErrNone);
	}

LOCAL_C TInt DeleteBackwardEntry(const TDesC& aName,TBool aIsDir)
//
// Deletes entry with aName and corresponding entries created for EChainBackwards
//
	{
	TInt r=TheFs.Delete(_L("\\fat\\file2"));
	test_Value(r, r==KErrNone||r==KErrNotFound);
	r=TheFs.Delete(_L("\\fat\\file3"));
	test_Value(r, r==KErrNone||r==KErrNotFound);
	if(aIsDir)
		r=TheFs.RmDir(aName);
	else
		r=TheFs.Delete(aName);
	return r;
	}

LOCAL_C TInt CreateBackwardEntry(const TDesC& aName,TBool aIsDir,TInt aSize)
//
// Creates an entry whose fat cluster chain first goes backwards(upto 3.5kb for fat16 file)
// and then forwards
//
	{
	TInt r=DeleteBackwardEntry(aName,aIsDir);
	test_Value(r, r==KErrNone||r==KErrNotFound);
	CreateAlternate(_L("\\fat\\file1"),_L("\\fat\\file2"));
	RFile entry;
	if(aIsDir)
		{
		r=TheFs.MkDir(aName);
		if(r!=KErrNone)
			return(r);
		}
	else
		{
		r=entry.Create(TheFs,aName,EFileShareAny);
		if(r!=KErrNone)
			return(r);
		r=entry.SetSize(1);
		test_KErrNone(r);
		}
	RFile file3;
	r=file3.Create(TheFs,_L("\\fat\\file3"),EFileShareAny);
	test_KErrNone(r);
	r=file3.SetSize(EntriesPerFatSector()*gBytesPerCluster);
	test_KErrNone(r);
	r=TheFs.Delete(_L("\\fat\\file1"));
	test_KErrNone(r);
	file3.Close();
	if(aIsDir)
		ExpandDirectory(aName,aSize);
	else
		{
		r=entry.SetSize(aSize);
		test_KErrNone(r);
		entry.Close();
		}
	return(KErrNone);	
	}

LOCAL_C TInt DeleteStdEntry(const TDesC& aName,TBool aIsDir)
//
// Deletes entry with aName
//
	{
	if(aIsDir)
		return(TheFs.RmDir(aName));
	else
		return(TheFs.Delete(aName));
	}

LOCAL_C TInt CreateStdEntry(const TDesC& aName,TBool aIsDir,TInt aSize)
//
// Creates entry with aName where the cluster chain grows contiguously
//
	{
	TInt r=DeleteStdEntry(aName,aIsDir);
	test_Value(r, r==KErrNone||r==KErrNotFound);
	if(aIsDir)
		{
		r=TheFs.MkDir(aName);
		if(r==KErrNone)
			ExpandDirectory(aName,aSize);
		return(r);
		}
	else
		{
		RFile file;
		r=file.Create(TheFs,aName,EFileShareAny);
		if(r==KErrNone)
			{
			r=file.SetSize(aSize);
			test_KErrNone(r);
			}
		else if(r==KErrAlreadyExists)
			{
			TInt res =file.Open(TheFs,aName,EFileShareAny);
			test_KErrNone(res);
			}
		else
			return(r);
		file.Close();
		return(r);
		}
	}

LOCAL_C TInt CreateEntry(const TDesC& aName,TBool aIsDir,TFatChain aChain,TInt aSize)
//
// Creates entry with aName whose fat cluster chain characteristics determined by aChain
//
	{
	switch(aChain)
		{
		case(EChainStd):return(CreateStdEntry(aName,aIsDir,aSize));
		case(EChainAlternate):return(CreateAlternateEntry(aName,aIsDir,aSize));
		case(EChainBackwards):return(CreateBackwardEntry(aName,aIsDir,aSize));
		case(EChainForwards):return(CreateForwardEntry(aName,aIsDir,aSize));
		default:return(KErrGeneral);
		}
	}

LOCAL_C TInt DeleteEntry(const TDesC& aName,TBool aIsDir,TFatChain aChain)
//
// Delete entry with aName
//
	{
	switch(aChain)
		{
		case(EChainStd):return(DeleteStdEntry(aName,aIsDir));
		case(EChainAlternate):return(DeleteAlternateEntry(aName,aIsDir));
		case(EChainBackwards):return(DeleteBackwardEntry(aName,aIsDir));
		case(EChainForwards):return(DeleteForwardEntry(aName,aIsDir));
		default:return(KErrGeneral);
		}
	}

LOCAL_C void TestRFsDelete(const TDesC& aName,TFatChain aChain,TInt aFileSize)
//
// test RFs::Delete
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Start(_L("TestRFsDelete"));
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aName,EFalse,aChain,aFileSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		r=TheFs.Delete(aName);
		if(r==KErrNone)
			break;
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		++failCount;
		}
	r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(!EntryExists(aName));
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFsRmDir(const TDesC& aName,TFatChain aChain,TInt aDirSize)
//
// test RFs::RmDir
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFsRmDir"));
	switch (aChain)
		{
		case EChainStd:
			RDebug::Print(_L("Chain Std %S size %d"), &aName, aDirSize);
			break;
		case EChainAlternate:
			RDebug::Print(_L("Chain Alternate %S size %d"), &aName, aDirSize);
			break;
		case EChainBackwards:
			RDebug::Print(_L("Chain Backwards %S size %d"), &aName, aDirSize);
			break;
		case EChainForwards:
			RDebug::Print(_L("Chain Forwards %S size %d"), &aName, aDirSize);
			break;
		default:
			break;
		}
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aName,ETrue,aChain,aDirSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		r=TheFs.RmDir(aName);
		if(r==KErrNone)
			break;
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		RDebug::Print(_L("%6d: ScanDrive = %d"), __LINE__, r);
		if (r != KErrNone)
		{
			RDebug::Print(_L("ScanDrive fail %d"), r);
			DumpFat();
			DumpData(NULL, 0, 200);
		}
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		RDebug::Print(_L("%6d: CheckDisk = %d"), __LINE__, r);
		test_KErrNone(r);
		++failCount;
		}
	r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(!EntryExists(aName));
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFsMkDir(const TDesC& aName)
//
// test RFs::MkDir
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFsMkDir"));
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=DeleteEntry(aName,ETrue,EChainStd);
		test_Value(r, r==KErrNone||r==KErrNotFound);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		r=TheFs.MkDir(aName);
		if(r==KErrNone)
			break;
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		++failCount;
		}
	r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(aName));
	r=DeleteEntry(aName,ETrue,EChainStd);
	test_KErrNone(r);
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFsRename(const TDesC& aOldName,const TDesC& aNewName,TBool aIsDir,TFatChain aChain,TInt aSize)
//
// test RFs::Rename
//
	{
	test.Next(_L("TestRFsRename"));
	TInt failCount=TheFailCount;
	TInt r;
	TEntry oldEntryInfo,newEntryInfo;
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aOldName,aIsDir,aChain,aSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		r=DeleteEntry(aNewName,aIsDir,aChain);
		test_Value(r, r==KErrNone||r==KErrNotFound);
		GetEntryDetails(aOldName,oldEntryInfo);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		r=TheFs.Rename(aOldName,aNewName);
		if(r==KErrNone)
			break;
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		// no start cluster if aSize==0
		if(aSize!=0)
			test(OneEntryExists(aOldName,aNewName));
		++failCount;
		}
	r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(aNewName) && !EntryExists(aOldName));
	GetEntryDetails(aNewName,newEntryInfo);
	test(IsSameEntryDetails(oldEntryInfo,newEntryInfo));
	r=DeleteEntry(aNewName,aIsDir,aChain);
	test_KErrNone(r);
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFsReplace(const TDesC& aOldName, const TDesC& aNewName,TBool aBothExist,TFatChain aChain,TInt aFileSize)
//
// test RFs::Replace
//
	{
	
	TInt failCount=TheFailCount;
	TInt r;
	if(aBothExist)
		test.Next(_L("TestRFsReplace with new name existing"));
	else
		test.Next(_L("TestRFsReplace with new name not existing"));
	TEntry oldEntryInfo,newEntryInfo;
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aOldName,EFalse,aChain,aFileSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		if(aBothExist)
			{
			r=CreateEntry(aNewName,EFalse,aChain,aFileSize);
			test_Value(r, r==KErrNone||r==KErrAlreadyExists);
			}
		else
			{
			r=DeleteEntry(aNewName,EFalse,aChain);
			test_Value(r, r==KErrNone||r==KErrNotFound);
			}
		GetEntryDetails(aOldName,oldEntryInfo);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		r=TheFs.Replace(aOldName,aNewName);
		if(r==KErrNone)
			break;
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		if(!aBothExist && aFileSize!=0)
			test(OneEntryExists(aOldName,aNewName));
		else if(aBothExist)
			test(EntryExists(aOldName)||EntryExists(aNewName));
		++failCount;
		}
	r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
	test_KErrNone(r);
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(aNewName) && !EntryExists(aOldName));
	GetEntryDetails(aNewName,newEntryInfo);
	test(IsSameEntryDetails(oldEntryInfo,newEntryInfo));
	r=DeleteEntry(aNewName,EFalse,aChain);
	test_KErrNone(r);
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFileCreate(const TDesC& aName)
//
// test RFile::Create
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFileCreate"));
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=DeleteEntry(aName,EFalse,EChainStd);
		test_Value(r, r==KErrNone||r==KErrNotFound);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		RFile file;
		r=file.Create(TheFs,aName,EFileShareAny);
		if(r==KErrNone)
			{
			r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
			test_KErrNone(r);
			file.Close();
			break;
			}
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		++failCount;
		}
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(aName));
	r=DeleteEntry(aName,EFalse,EChainStd);
	test_KErrNone(r);
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFileTemp(const TDesC& aPath)
//
// test RFile::Temp
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFileTemp"));
	TFileName temp;
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		RFile file;
 		r=file.Temp(TheFs,aPath,temp,EFileShareAny);
		if(r==KErrNone)
			{
			r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
			test_KErrNone(r);
			file.Close();
			break;
			}
		test(r==WriteFailValue);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		++failCount;
		}
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(temp));
	r=DeleteEntry(temp,EFalse,EChainStd);
	test_KErrNone(r);
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFileRename(const TDesC& aOldName, const TDesC& aNewName,TFatChain aChain,TInt aFileSize)
//
// test RFile::Rename
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFileRename"));
	TEntry oldEntryInfo,newEntryInfo;
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aOldName,EFalse,aChain,aFileSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		r=DeleteEntry(aNewName,EFalse,aChain);
		test_Value(r, r==KErrNone||r==KErrNotFound);
		GetEntryDetails(aOldName,oldEntryInfo);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		RFile file;
		r=file.Open(TheFs,aOldName,EFileShareExclusive|EFileWrite);
		test_KErrNone(r);
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		r=file.Rename(aNewName);
		if(r==KErrNone)
			{
			r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
			test_KErrNone(r);
			file.Close();
			break;
			}
		test_Equal(WriteFailValue,r);
		file.Close();
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		if(aFileSize)
			test(OneEntryExists(aOldName,aNewName));
		++failCount;
		}
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(aNewName) && !EntryExists(aOldName));
	GetEntryDetails(aNewName,newEntryInfo);
	test(IsSameEntryDetails(oldEntryInfo,newEntryInfo));
	r=DeleteEntry(aNewName,EFalse,aChain);
	test_KErrNone(r);
	++TheOpNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFileReplace(const TDesC& aName,TBool aAlreadyExists,TFatChain aChain,TInt aFileSize)
//
// test RFile::Replace
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFileReplace"));
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		if(aAlreadyExists)
			{
			r=CreateEntry(aName,EFalse,aChain,aFileSize);
			test_Value(r, r==KErrNone||r==KErrAlreadyExists);
			}
		else
			{
			r=DeleteEntry(aName,EFalse,aChain);
			test_Value(r, r==KErrNone||r==KErrNotFound);
			}
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		RFile file;
		r=file.Replace(TheFs,aName,EFileShareAny);
		if(r==KErrNone)
			{
			r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
			test_KErrNone(r);
			file.Close();
			break;
			}
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		++failCount;
		}
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	test(EntryExists(aName));
	r=DeleteEntry(aName,EFalse,aChain);
	test_KErrNone(r);
	if(!aAlreadyExists)
		{
		++TheOpNumber;
		TheFailCount=0;
		}
	else
		{
		++TheFunctionNumber;
		TheOpNumber=TheFailCount=0;
		}
	}

LOCAL_C void TestRFileSetSize(const TDesC& aName,TFatChain aChain,TInt aOldFileSize,TInt aNewFileSize)
//
// test RFile::SetSize
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFileSetSize"));
	test.Printf(_L("old size=%d new size=%d\n"),aOldFileSize,aNewFileSize);
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aName,EFalse,aChain,aOldFileSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		RFile file;
		r=file.Open(TheFs,aName,EFileShareAny|EFileWrite);
		test_KErrNone(r);
		r=file.SetSize(aNewFileSize);
		// close the file before testing the return value!
		file.Close();
		if(r==KErrNone)
			{
			r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
			test_KErrNone(r);
			file.Close();
			break;
			}
		file.Close();
		test_Equal(WriteFailValue,r);
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		r=file.Open(TheFs,aName,EFileShareAny|EFileWrite);
		test_KErrNone(r);
		TInt size;
		r=file.Size(size);
		test_KErrNone(r);
		test_Value(size, size==aNewFileSize||size==aOldFileSize);
		file.Close();
		++failCount;
		}
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	RFile file;
	r=file.Open(TheFs,aName,EFileShareAny);
	test_KErrNone(r);
	TInt fileSize;
	r=file.Size(fileSize);
	test_KErrNone(r);
	test_Equal(aNewFileSize,fileSize);
	file.Close();
	r=DeleteEntry(aName,EFalse,aChain);
	test_KErrNone(r);
	++TheFunctionNumber;
	TheFailCount=0;
	}

LOCAL_C void TestRFileWrite(const TDesC& aName,TFatChain aChain,TInt aFileSize,TInt aPos,TInt aLength)
//
// test RFile::Write
//
	{
	TInt failCount=TheFailCount;
	TInt r;
	test.Next(_L("TestRFileWrite"));
	test.Printf(_L("aFileSize=%d,aPos=%d,aLength=%d\n"),aFileSize,aPos,aLength);
	TInt newSize=(aFileSize>=aPos+aLength)?aFileSize:aPos+aLength;
	HBufC8* desPtr;
	desPtr=HBufC8::New(aLength);
	test_NotNull(desPtr);
	TPtr8 des=desPtr->Des();
	des.SetLength(aLength);
	InitialiseWriteBuffer(des);
	FOREVER
		{
		test.Printf(_L("failCount=%d\n"),failCount);
		r=CreateEntry(aName,EFalse,aChain,aFileSize);
		test_Value(r, r==KErrNone||r==KErrAlreadyExists);
		if(IsReset)
			{
			++TheFailCount;
			WriteLogFile();
			}
		r=SetWriteFailOn(failCount);
		test_KErrNone(r);
		RFile file;
		r=file.Open(TheFs,aName,EFileShareAny|EFileWrite);
		test_KErrNone(r);
		r=file.Write(aPos,des,aLength);
		if(r==KErrNone)
			{
			r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoWriteFailOff);
			test_KErrNone(r);
			file.Close();
			break;
			}
		test_Equal(WriteFailValue,r);
		file.Close();
		r=TheFs.ScanDrive(gSessionPath);
		test_KErrNone(r);
		r=TheFs.CheckDisk(gSessionPath);
		test_KErrNone(r);
		file.Open(TheFs,aName,EFileShareAny);
		test_KErrNone(r);
		TInt fileSize;
		r=file.Size(fileSize);
		// with fair scheduling enabled it's possible for the file 
		// size to grow even if the write appears to have failed...
//		test_Value(fileSize, fileSize==aFileSize||fileSize==newSize);
		test_Value(fileSize, fileSize>=aFileSize && fileSize <= newSize);

		file.Close();
		++failCount;
		}
	r=TheFs.CheckDisk(gSessionPath);
	test_KErrNone(r);
	RFile file;
	r=file.Open(TheFs,aName,EFileShareAny);
	test_KErrNone(r);
	TInt fileSize;
	r=file.Size(fileSize);
	test_KErrNone(r);
	test_Equal(newSize,fileSize);
	HBufC8* desPtr2;
	desPtr2=HBufC8::New(aLength);
	test_NotNull(desPtr2);
	TPtr8 des2=desPtr2->Des();
	des2.SetLength(aLength);
	r=file.Read(aPos,des2,des2.Length());
	test_KErrNone(r);
	r=des2.Compare(des);
	test_KErrNone(r);
	file.Close();
	r=DeleteEntry(aName,EFalse,aChain);
	test_KErrNone(r);
	delete desPtr;
	delete desPtr2;
	++TheFunctionNumber;
	TheFailCount=0;
	}


LOCAL_C void TestOperations(const TDesC& aOldName,const TDesC& aNewName,TFatChain aChain,TInt aFileSize, TInt aDirClusters)
//
// Tests the specified operations
//
	{
	TFileName oldDirName=aOldName;
	TFileName newDirName=aNewName;
	// create directory for directory operations
	oldDirName+=_L("\\");
	newDirName+=_L("\\");
	// locate path for RFile::Temp
	TInt pathPos=aOldName.LocateReverse('\\')+1;
	TFileName tempPath=aOldName.Left(pathPos);
	test.Printf(_L("aOldName=%S\n"),&aOldName);
	test.Printf(_L("aNewName=%S\n"),&aNewName);
	test.Printf(_L("tempPath=%S\n"),&tempPath);
	switch(TheOpNumber)
		{
		case(0):TestRFsDelete(aOldName,aChain,aFileSize);
		case(1):TestRFsRmDir(oldDirName,aChain,aDirClusters);
		case(2):TestRFsMkDir(oldDirName);
		case(3):TestRFsRename(aOldName,aNewName,EFalse,aChain,aFileSize);
		case(4):TestRFsRename(oldDirName,newDirName,ETrue,aChain,aDirClusters);
		case(5):TestRFsReplace(aOldName,aNewName,EFalse,aChain,aFileSize);
		case(6):TestRFsReplace(aOldName,aNewName,ETrue,aChain,aFileSize);
		case(7):TestRFileCreate(aOldName);
		case(8):TestRFileTemp(tempPath);
		case(9):TestRFileRename(aOldName,aNewName,aChain,aFileSize);
		case(10):TestRFileReplace(aOldName,EFalse,aChain,aFileSize);
		case(11):TestRFileReplace(aOldName,ETrue,aChain,aFileSize);break;
		default:test(EFalse);
		}
	test.End();
	}

LOCAL_C void TestOperation0()
//
//
//
	{
	// tests entries in root directory
	test.Next(_L("TestOperation0"));
	TestOperations(_L("\\entryWithTwoVfats"),_L("\\anotherEntryWithTwo"),EChainStd,0,0);
	}
	
LOCAL_C void TestOperation1()
//
//
//
	{
	// tests entries in a full root directory
	test.Next(_L("TestOperation1"));
	if(TheFailCount==0)
		FillUpRootDir(4);
	TestOperations(_L("\\entryOne"),_L("\\entryTwo"),EChainStd,512,0);
	UnFillUpRootDir(4);
	}

LOCAL_C void TestOperation2()
//
//
//
	{
	// tests entries in same subdir
	test.Next(_L("TestOperation2"));
	TestOperations(_L("\\test\\subdir1\\NameWithFourVFatEntriesWaffle"),_L("\\test\\subdir1\\aEntry"),EChainAlternate,5120,1);
	}


LOCAL_C void TestOperation3()
//
//
//
	{
	// tests entries in different subdir
	test.Next(_L("TestOperation3"));
	TestOperations(_L("\\test\\subdir1\\NameWithThreeEntries"),_L("\\ANother\\aEntrytwo"),EChainAlternate,15000,10);
	}


LOCAL_C void TestOperation4()
//
//
//
	{
	// tests entries with cluster chain of EChainForwards
	test.Next(_L("TestOperation4"));
	TestOperations(_L("\\test\\subdir1\\aEntry"),_L("\\aEntry"),EChainForwards,12799,25);
	}


LOCAL_C void TestOperation5()
//
//
//
	{
	// tests entries with cluster chain of EChainBackwards
	test.Next(_L("TestOperation5"));
	TestOperations(_L("\\test\\subdir1\\aEntry"),_L("\\ANother\\EntrywithThree"),EChainBackwards,51199,10);
	}

LOCAL_C void TestOperation6()
//
//
//
	{
	// tests entries where old name has a very long name
	test.Next(_L("TestOperation6"));
	TFileName longName=_L("\\test\\subdir1\\");
	MakeVeryLongName(longName);
	TestOperations(longName,_L("\\ANother\\e1"),EChainAlternate,5100,0);
	}

LOCAL_C void TestOperation7()
//
//
//
	{
	// tests entries where new name fills up subdir cluster
	test.Next(_L("TestOperation7"));
	TFileName name=_L("\\test\\subdir2\\");
	// add entry with 7 vfat entries
	MakeEntryName(name,80);
	if(TheFailCount==0)
		CreateEntry(name,EFalse,EChainStd,1);
	TestOperations(_L("\\test\\subdir2\\EntryWithThree"),_L("\\test\\subdir2\\EntryWithThree-"),EChainStd,512,0);
	DeleteEntry(name,EFalse,EChainStd);
	}

LOCAL_C void TestOperation8()
//
//
//
	{
	// tests entries where new name is first entry in new subdir cluster
	test.Next(_L("TestOperation8"));
	TFileName name=_L("\\test\\subdir2\\");
	// add entry with 10 vfat entries
	MakeEntryName(name,125);
	if(TheFailCount==0)
		CreateEntry(name,EFalse,EChainStd,175000);
	TestOperations(_L("\\test\\subdir2\\Entrywith3three"),_L("\\test\\subdir2\\entrywiththree-"),EChainStd,512,1);
	DeleteEntry(name,EFalse,EChainStd);
	}

GLDEF_C void DoTests()
	{
	TInt r;
	if(!IsReset && IsInternalRam())
		{
		test.Printf(_L("Error: Internal ram drive not tested\n"));
		return;
		}
	if(!IsReset)
		QuickFormat();

	DoReadBootSector();
	DumpBootSector();
	ClearDiskData();

	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	switch(TheFunctionNumber)
		{
		case(0):TestOperation0();
		case(1):{
				TestOperation1();
				r=TheFs.MkDir(_L("\\fat\\"));
				test_KErrNone(r);
				r=TheFs.MkDir(_L("\\test\\"));
				test_KErrNone(r);
				r=TheFs.MkDir(_L("\\ANother\\"));
				test_KErrNone(r);
				r=TheFs.MkDir(_L("\\test\\subdir1\\"));
				test_KErrNone(r);
				r=TheFs.MkDir(_L("\\test\\subdir2\\"));
				test_KErrNone(r);}
		case(2):{
				TestOperation2();
				// add some filler files
				CreateEntry(_L("\\test\\subdir1\\FillerOne"),EFalse,EChainStd,512);
				CreateEntry(_L("\\test\\subdir1\\FillerTwo"),EFalse,EChainStd,1024);}
		case(3):TestOperation3();
		case(4):{
				TestOperation4();
				// add some filler files
				CreateEntry(_L("\\ANother\\FillerThree"),EFalse,EChainStd,1536);
				CreateEntry(_L("\\test\\subdir1\\FillerFour"),EFalse,EChainStd,2048);}
		case(5):TestOperation5();
		case(6):TestOperation6();
		case(7):TestOperation7();
		case(8):TestOperation8();
		// increase size of file
		case(9):TestRFileSetSize(_L("\\entry1"),EChainStd,0,512);
		case(10):TestRFileSetSize(_L("\\entry1"),EChainAlternate,0,1025);
		case(11):TestRFileSetSize(_L("\\entry1"),EChainStd,1,512);
		// seek index (of CFatFileCB) resized
		case(12):TestRFileSetSize(_L("\\entry1"),EChainForwards,512,66666);
		// seek index resized
		case(13):TestRFileSetSize(_L("\\entry1"),EChainBackwards,32779,131074);
		// decrease size of file
		// seek index resized
		case(14):TestRFileSetSize(_L("\\entry1"),EChainForwards,133000,32768);
		// seek index resized
		case(15):TestRFileSetSize(_L("\\entry1"),EChainBackwards,65536,1);
		// seek index resized
		case(16):TestRFileSetSize(_L("\\entry1"),EChainAlternate,66554,0);
		case(17):TestRFileSetSize(_L("\\entry1"),EChainStd,1024,1);
		case(18):TestRFileSetSize(_L("\\entry1"),EChainAlternate,512,0);
		case(19):TestRFileWrite(_L("\\entry2"),EChainStd,0,0,512);
		case(20):TestRFileWrite(_L("\\entry2"),EChainAlternate,5120,512,1024);
		case(21):TestRFileWrite(_L("\\entry2"),EChainForwards,3584,3584,5000);
		case(22):TestRFileWrite(_L("\\entry2"),EChainBackwards,3000,2999,2000);
		// seek index resized
		case(23):TestRFileWrite(_L("\\entry2"),EChainBackwards,64000,64000,3000);
		// seek index resized
		case(24):TestRFileWrite(_L("\\entry2"),EChainForwards,131072,2,4000);break;
		default:test(EFalse);
		}
	DeleteEntry(_L("\\test\\subdir1\\FillerFour"),EFalse,EChainStd);
	DeleteEntry(_L("\\ANother\\FillerThree"),EFalse,EChainStd);
	DeleteEntry(_L("\\test\\subdir1\\FillerTwo"),EFalse,EChainStd);
	DeleteEntry(_L("\\test\\subdir1\\FillerOne"),EFalse,EChainStd);
	r=TheFs.RmDir(_L("\\test\\subdir2\\"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("\\test\\subdir1\\"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("\\ANother\\"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("\\test\\"));
	test_KErrNone(r);
	r=TheFs.RmDir(_L("\\fat\\"));
	test_KErrNone(r);
	if (gFatBuf)
		{
		delete gFatBuf;
		gFatBuf = NULL;
		}
	}
#endif
