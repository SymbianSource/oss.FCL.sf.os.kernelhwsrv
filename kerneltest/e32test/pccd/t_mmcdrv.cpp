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
// e32test\pccd\t_mmcdrv.cpp
// Test the MultiMediaCard (MMC) media driver
// Spare Test case Numbers 0513-0519
// 
//

#include "../mmu/d_sharedchunk.h"
#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <f32fsys.h>
#include <e32def.h>
#include <e32def_private.h>

const TInt KDiskSectorSize=512;
const TInt KDiskSectorShift=9;
const TUint KDiskSectorMask=0xFFFFFE00;
const TInt KSectBufSizeInSectors=8;
const TInt KSectBufSizeInBytes=(KSectBufSizeInSectors<<KDiskSectorShift);
const TInt KRdWrBufLen=(KSectBufSizeInBytes+KDiskSectorSize); // 4.5K - exceeds driver local buffer size

const TInt KShortFormatInSectors=1;
const TInt KShortFormatInBytes=(KShortFormatInSectors<<KDiskSectorShift);
const TInt KLongFormatInSectors=KSectBufSizeInSectors+1;	// 4.5K - exceeds driver local buffer size
const TInt KLongFormatInBytes=(KLongFormatInSectors<<KDiskSectorShift);

const TInt KVeryLongSectBufSizeInSectors=4096;												// ..2M
const TInt KVeryLongSectBufSizeInBytes=(KVeryLongSectBufSizeInSectors<<KDiskSectorShift);	//
const TInt KVeryLongRdWrBufLen=(KVeryLongSectBufSizeInBytes+KDiskSectorSize);				// 2M + 0.5K

const TInt KHeapSize=0x4000;

const TInt64 KDefaultRandSeed = MAKE_TINT64(0x501a501a, 0x501a501a);

#define TEST_DOOR_CLOSE 	0					// see comment in E32Main()


class TMMCDrive : public TBusLocalDrive
	{
public:
	enum TTestMode
		{
		ETestPartition,
		ETestWholeMedia,
		ETestSharedMemory,
		ETestSharedMemoryCache,
		ETestSharedMemoryFrag,
		ETestSharedMemoryFragCache,
		EMaxTestModes
		};
public:
	TMMCDrive();
	
	TInt Read(TInt64 aPos, TInt aLength, TDes8& aTrg);
	TInt Write(TInt64 aPos, const TDesC8& aSrc);

	TInt SetTestMode(TTestMode aTestMode);
	TTestMode TestMode();

	void SetSize(TInt64 aDriveSize, TInt64 aMediaSize);
	TInt64 Size();
private:
	TTestMode iTestMode;

	TInt64 iDriveSize;
	TInt64 iMediaSize;
	};

// Serial numbers for 'special case' test cards (ie - those with known problems)
class TKnownCardTypes
	{
public:
	enum TCardType
		{
		EStandardCard = 0,
		EBuffalloMiniSD_32M_ERASE,
		EBuffalloMiniSD_64M_ERASE,
		EBuffalloMiniSD_128M_ERASE,
		EBuffalloMiniSD_256M_ERASE,
		EBuffalloMiniSD_512M_ERASE,
		EBuffalloMiniSD_512M,
		EIntegralHSSD_2G,
		ESanDiskMmcMobile_1GB
		};

	TKnownCardTypes(TCardType aCardType, const TText8* aSerialNumber) 
		: iCardType(aCardType), iSerialNumber(aSerialNumber) {};

	TCardType iCardType;
	const TText8* iSerialNumber;
	};

LOCAL_D TKnownCardTypes KnownCardTypes[] = 	
	{
	//** The Following Buffalo Cards all have a known Mis-Implementation
	// When requesting Erase the area to be erase is specified in terms of a start (CMD32) and stop (CMD33) blocks
	// Specification states that CMD33 refers to the end block in terms of the first byte of that block
	// the Buffallo implementation requires that the last byte of the block is specified.
	
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_32M_ERASE,  _S8("936300c70e150d003630333046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_64M_ERASE,  _S8("d96600456d120a003732343046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_128M_ERASE, _S8("f964000d13150c003630333046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_256M_ERASE, _S8("4d66004c68120a003732343046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_512M_ERASE, _S8("db6500824e0010013236333243454228")),
	
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_32M_ERASE,  _S8("df6400e60d150d003630333046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_64M_ERASE,  _S8("296600386d120a003732343046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_128M_ERASE, _S8("b16400f512150c003630333046445004")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_256M_ERASE, _S8("435600cc390000000000004453474b13")),
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_512M_ERASE, _S8("ed6300de700000000000004453474b13")),
	//***********************************************************************************************//
	
	TKnownCardTypes(TKnownCardTypes::EBuffalloMiniSD_512M, _S8("0d56004e2d0000000000004453474b13")),
	TKnownCardTypes(TKnownCardTypes::EIntegralHSSD_2G,     _S8("37570058073099114732304453000027")),
	TKnownCardTypes(TKnownCardTypes::ESanDiskMmcMobile_1GB,_S8("956a1c00001810303030303030000015"))
	};


LOCAL_D RTest test(_L("T_MMCDRV"));
LOCAL_D RTest nTest(_L("This thread doesn't disconnect"));
LOCAL_D TBool ChangeFlag;
LOCAL_D TBool SecThreadChangeFlag;


LOCAL_D TPtr8 wrBuf(NULL, KVeryLongRdWrBufLen);
LOCAL_D TPtr8 rdBuf(NULL, KVeryLongRdWrBufLen);
LOCAL_D HBufC8* wrBufH = NULL;
LOCAL_D HBufC8* rdBufH = NULL;

LOCAL_D TInt DriveNumber = -1; // Local Drive number
LOCAL_D TInt RFsDNum = -1;	// File Server Drive number
LOCAL_D TMMCDrive TheMmcDrive;
LOCAL_D TLocalDriveCapsV5Buf DriveCaps;
LOCAL_D TKnownCardTypes::TCardType CardType;
LOCAL_D TBool IsReadOnly;

LOCAL_D RSharedChunkLdd Ldd;
LOCAL_D RChunk TheChunk;
LOCAL_D TInt PageSize;
const TUint ChunkSize    = 0x201000;	//2MB+4096bytes > than largest transfer

const TInt	 KSingSectorNo=1;
const TInt64 KTwoGigbytes = 0x80000000;

TBool mediaChangeSupported=EFalse; // ???
TBool ManualMode=EFalse;

// Wrappers for the test asserts
GLREF_C void TestIfError( TInt aValue, TInt aLine, const TText* aFile );
GLREF_C void TestIfErrorMsg( TInt aValue, TInt aLine, const TText* aFile, const TDesC& aMessageOnError );
GLREF_C void TestEqual( TInt aValue, TInt aExpected, TInt aLine, const TText* aFile );
GLREF_C void TestEqualMsg( TInt aValue, TInt aExpected, TInt aLine, const TText* aFile, const TDesC& aMessageOnError );
GLREF_C void TestEitherEqual( TInt aValue, TInt aExpected1, TInt aExpected2, TInt aLine, const TText* aFile );
GLREF_C void TestRange( TInt aValue, TInt aMin, TInt Max, TInt aLine, const TText* aFile );

#define TEST_FOR_ERROR2( r, l, f )	TestIfError( r, l, _S(f) )
#define TEST_FOR_ERROR_ERRMSG2( r, l, f, m )	TestIfErrorMsg( r, l, _S(f), m )
#define TEST_FOR_VALUE2( r, e, l, f )	TestEqual( r, e, l, _S(f) )
#define TEST_FOR_VALUE_ERRMSG2( r, e, l, f, m )	TestEqualMsg( r, e, l, _S(f), m )
#define TEST_FOR_EITHER_VALUE2( r, e1, e2, l, f )	TestEitherEqual( r, e1, e2, l, _S(f) )
#define TEST_FOR_RANGE2( r, min, max, l, f )	TestRange( r, min, max, l, _S(f) )

#define TEST_FOR_ERROR( r )	TEST_FOR_ERROR2( r, __LINE__, __FILE__ )
#define TEST_FOR_ERROR_ERRMSG( r, m )	TEST_FOR_ERRORMSG2( r, __LINE__, __FILE__, m )
#define TEST_FOR_VALUE( r, expected )	TEST_FOR_VALUE2( r, expected, __LINE__, __FILE__ )
#define TEST_FOR_VALUE_ERRMSG( r, expected, m )	TEST_FOR_VALUE_ERRMSG2( r, expected, __LINE__, __FILE__, m )
#define TEST_FOR_EITHER_VALUE( r, expected1, expected2 )	TEST_FOR_EITHER_VALUE2( r, expected1, expected2, __LINE__, __FILE__ )
#define TEST_FOR_RANGE( r, min, max )	TEST_FOR_RANGE2( r, min, max, __LINE__, __FILE__ )

GLDEF_C void TestIfError( TInt aValue, TInt aLine, const TText* aFile )
	{
	if( aValue < 0 )
		{
		_LIT( KErrorTestFailMsg, "ERROR %d\n\r" );
		test.Printf( KErrorTestFailMsg, aValue );
		test.operator()( EFalse, aLine, (const TText*)(aFile) );
		}
	}

GLDEF_C void TestIfErrorMsg( TInt aValue, TInt aLine, const TText* aFile, const TDesC& aMessageOnError )
	{
	if( aValue < 0 )
		{
		_LIT( KErrorTestFailMsg, "ERROR %d %S\n\r" );
		test.Printf( KErrorTestFailMsg, aValue, &aMessageOnError );
		test.operator()( EFalse, aLine, (const TText*)(aFile) );
		}
	}


GLDEF_C void TestEqual( TInt aValue, TInt aExpected, TInt aLine, const TText* aFile )
	{
	if( aExpected != aValue )
		{
		_LIT( KEqualTestFailMsg, "ERROR %d expected %d\n\r" );
		test.Printf( KEqualTestFailMsg, aValue, aExpected );
		test.operator()( EFalse, aLine, (const TText*)(aFile) );
		}
	}

GLDEF_C void TestEqualMsg( TInt aValue, TInt aExpected, TInt aLine, const TText* aFile, const TDesC& aMessageOnError )
	{
	if( aExpected != aValue )
		{
		_LIT( KEqualTestFailMsg, "ERROR %d expected %d %S\n\r" );
		test.Printf( KEqualTestFailMsg, aValue, aExpected, &aMessageOnError );
		test.operator()( EFalse, aLine, (const TText*)(aFile) );
		}
	}

GLDEF_C void TestEitherEqual( TInt aValue, TInt aExpected1, TInt aExpected2, TInt aLine, const TText* aFile )
	{
	if( (aExpected1 != aValue) && (aExpected2 != aValue) )
		{
		_LIT( KEqualTestFailMsg, "ERROR %d expected %d or %d\n\r" );
		test.Printf( KEqualTestFailMsg, aValue, aExpected1, aExpected2 );
		test.operator()( EFalse, aLine, (const TText*)(aFile) );
		}
	}

GLDEF_C void TestRange( TInt aValue, TInt aMin, TInt aMax, TInt aLine, const TText* aFile )
	{
	if( (aValue < aMin) || (aValue > aMax) )
		{
		_LIT( KRangeTestFailMsg, "ERROR 0x%x expected 0x%x..0x%x\n\r" );
		test.Printf( KRangeTestFailMsg, aValue, aMin, aMax );
		test.operator()( EFalse, aLine, (const TText*)(aFile) );
		}
	}

////

TMMCDrive::TMMCDrive()
  : iTestMode(ETestPartition),
    iDriveSize(0),
    iMediaSize(0)
	{
	}

TInt TMMCDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
	{
	if(iTestMode == ETestWholeMedia)
		{
		return TBusLocalDrive::Read(aPos, aLength, &aTrg, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
		}
	else if(iTestMode != ETestPartition && aLength <= (TInt)ChunkSize)
		{
		TPtr8 wholeBufPtr(TheChunk.Base(),aLength);
	
		TInt r = TBusLocalDrive::Read(aPos, aLength, wholeBufPtr);
	
		aTrg.Copy(wholeBufPtr);
		return r;
		}
	
	return TBusLocalDrive::Read(aPos, aLength, aTrg);
	}

TInt TMMCDrive::Write(TInt64 aPos,const TDesC8& aSrc)
	{
	if(iTestMode == ETestWholeMedia)
		{
		return TBusLocalDrive::Write(aPos, aSrc.Length(), &aSrc, KLocalMessageHandle, 0, RLocalDrive::ELocDrvWholeMedia);
		}
	else if(iTestMode != ETestPartition && aSrc.Length() <= (TInt)ChunkSize)
		{		
		TPtr8 wholeBufPtr(TheChunk.Base(),aSrc.Length());
		wholeBufPtr.Copy(aSrc);
	
		TInt r = TBusLocalDrive::Write(aPos, wholeBufPtr);
		
		return r;
		}
		
	return TBusLocalDrive::Write(aPos, aSrc);
	}

TInt TMMCDrive::SetTestMode(TTestMode aTestMode)
	{
	switch (aTestMode) 
		{
		case ETestWholeMedia   : 		test.Printf(_L("\nTesting Whole Media\n")); break;
		case ETestPartition    : 		test.Printf(_L("\nTesting Partition\n")); break;
		case ETestSharedMemory : 		test.Printf(_L("\nTesting Shared Memory\n")); break;
		case ETestSharedMemoryCache : 	test.Printf(_L("\nTesting Shared Memory (Caching)\n")); break;
		case ETestSharedMemoryFrag : 	test.Printf(_L("\nTesting Shared Memory (Fragmented)\n")); break;
		default :           			test.Printf(_L("\nTesting Shared Memory (Fragmented/Caching)\n")); break;
		}

	if(aTestMode == ETestWholeMedia && iMediaSize == 0)
		{
		test.Printf(_L("...not supported"));
		return KErrNotSupported;
		}

	iTestMode = aTestMode;
	return KErrNone;
	}

TMMCDrive::TTestMode TMMCDrive::TestMode()
	{
	return iTestMode;
	}

void TMMCDrive::SetSize(TInt64 aDriveSize, TInt64 aMediaSize)
	{
	iDriveSize = aDriveSize;
	iMediaSize = aMediaSize;
	}

TInt64 TMMCDrive::Size()
	{
	switch (iTestMode)
		{
		case ETestWholeMedia : return iMediaSize;
		default 			 : return iDriveSize;
		}
	}

//////

GLDEF_C void DumpBuffer( const TDesC8& aBuffer )
	/**
	 * Dump the content of aBuffer in hex
	 */
	{
	static const TText hextab[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
										'A', 'B', 'C', 'D', 'E', 'F' };
	const TInt KBytesPerLine = 32;
	const TInt KCharsPerLine = KBytesPerLine * 2;

	TInt remaining = aBuffer.Length();
	TUint8* pSrc = const_cast<TUint8*>(aBuffer.Ptr());

	TBuf<KCharsPerLine> line;
	line.SetLength( KCharsPerLine );	// don't need to print trailing space
	TInt bytesPerLine = KBytesPerLine;
	TInt lineOffs = 0;
	while( remaining )
		{
		if( remaining < KBytesPerLine )
			{
			bytesPerLine = remaining;
			line.SetLength( (bytesPerLine*2) );
			}
		TUint16* pDest = const_cast<TUint16*>(line.Ptr());
		remaining -= bytesPerLine;
		for( TInt i = bytesPerLine; i > 0; --i )
			{
			TUint8 c = *pSrc++;
			*pDest++ = hextab[c >> 4];
			*pDest++ = hextab[c & 0xF];
			}
		_LIT( KFmt, "%06x: %S\n\r" );
		test.Printf( KFmt, lineOffs, &line );
		lineOffs += bytesPerLine;
		}
	}


GLDEF_C TBool CompareBuffers( const TDesC8& aBuf1, const TDesC8& aBuf2 )
	{
	TInt count = 32;
	if (aBuf1.Length() < count) 
		count = aBuf1.Length();

	
	for (TInt i = 0; i < (aBuf1.Length()-count); i+= count)
		{
		if( aBuf1.Mid(i,count).Compare(aBuf2.Mid(i,count)) != 0)
			{
			// now need to find where mismatch ends
			TInt j =i;
			for (; j <= (aBuf1.Length()-count); j+= count)
				{
				if( aBuf1.Mid(j,count).Compare(aBuf2.Mid(j,count)) == 0) break;
				}
			test.Printf(_L("buf1 len: %d, buf2 len: %d\n"),aBuf1.Length(),aBuf2.Length());
			test.Printf( _L("Buffer mismatch @%d to %d (%d Bytes)\n\r"),i,j, (j-i) );
			test.Printf( _L("buffer 1 ------------------\n\r") );
			DumpBuffer( aBuf1.Mid(i,(j-i)) );
			test.Printf( _L("buffer 2 ------------------\n\r") );
			DumpBuffer( aBuf2.Mid(i,(j-i)) );
			test.Printf(_L("buf1 len: %d, buf2 len: %d\n"),aBuf1.Length(),aBuf2.Length());
			test.Printf( _L("Buffer mismatch @%d to %d (%d Bytes)\n\r"),i,j, (j-i) );
			return EFalse;
			}
		}
	return ETrue;
	}


void singleSectorRdWrTest(TInt aSectorOffset,TInt aLen)
//
// Perform a write / read test on a single sector (KSingSectorNo). Verify that the
// write / read back is successful and that the rest of the sector is unchanged.
//
	{

	TBuf8<KDiskSectorSize> saveBuf;
	test.Start(_L("Single sector write/read test"));
	test(aSectorOffset+aLen<=KDiskSectorSize);

	// Now save state of sector before we write to it
	TInt secStart=(KSingSectorNo<<KDiskSectorShift);
	test(TheMmcDrive.Read(secStart,KDiskSectorSize,saveBuf)==KErrNone);

	// Write zero's to another sector altogether (to ensure drivers 
	// local buffer hasn't already got test pattern we expect).
	wrBuf.Fill(0,KDiskSectorSize);
	test(TheMmcDrive.Write((KSingSectorNo+4)<<KDiskSectorShift,wrBuf)==KErrNone);

	// Write / read back sector in question
	wrBuf.SetLength(aLen);
	for (TInt i=0;i<aLen;i++)
		wrBuf[i]=(TUint8)(0xFF-i);
	test(TheMmcDrive.Write((secStart+aSectorOffset),wrBuf)==KErrNone);
	rdBuf.Fill(0,aLen);
	test(TheMmcDrive.Read((secStart+aSectorOffset),aLen,rdBuf)==KErrNone);
	test(CompareBuffers(rdBuf, wrBuf));
	//test(rdBuf.Compare(wrBuf)==0);

	// Now check the rest of the sector is unchanged
	rdBuf.Fill(0,KDiskSectorSize);
	test(TheMmcDrive.Read(secStart,KDiskSectorSize,rdBuf)==KErrNone);
	saveBuf.Replace(aSectorOffset,aLen,wrBuf);
	test(CompareBuffers(rdBuf, saveBuf));
	test.End();
	}

const TInt KMultSectorNo=2; 

void MultipleSectorRdWrTestMB(TInt aFirstSectorOffset, TInt aLen, TBool aWrMB, TBool aRdMB)
//
// Perform a write / read test over multiple sectors (starting within sector KMultSectorNo).
// Verify that the write / read back is successful and that the remainder of the first and
// last sectors are not affected.
//
	{

	TBuf8<KDiskSectorSize> saveBuf1;
	TBuf8<KDiskSectorSize> saveBuf2;

	test.Printf(_L("   MBW[%d] : MBR[%d]\n\r"), aWrMB, aRdMB);
	
	test(aFirstSectorOffset<KDiskSectorSize&&aLen<=KVeryLongRdWrBufLen);

	// If not starting on sector boundary then save 1st sector to check rest of 1st sector is unchanged
	TInt startSecPos=(KMultSectorNo<<KDiskSectorShift);
	if (aFirstSectorOffset!=0)
		test(TheMmcDrive.Read(startSecPos,KDiskSectorSize,saveBuf1)==KErrNone);

	// If not ending on sector boundary then save last sector to check rest of last sector is unchanged
	TInt endOffset=(aFirstSectorOffset+aLen)&(~KDiskSectorMask);
	TInt endSecPos=((startSecPos+aFirstSectorOffset+aLen)&KDiskSectorMask);
	if (endOffset)
		{
		test(TheMmcDrive.Read(endSecPos,KDiskSectorSize,saveBuf2)==KErrNone);
		}

	// Write zero's to another sector altogether (to ensure drivers 
	// local buffer hasn't already got test pattern we expect).
	wrBuf.Fill(0,KSectBufSizeInBytes);
	test(TheMmcDrive.Write((endSecPos+(2*KDiskSectorSize)),wrBuf)==KErrNone);
	
	TInt i;

	wrBuf.SetLength(aLen);
	for (i=0;i<aLen;i++)
		{
		wrBuf[i]=(TUint8)(0xFF-i);
		}

	if(aWrMB)
		{
		test(TheMmcDrive.Write((startSecPos+aFirstSectorOffset),wrBuf)==KErrNone);
		}
	else
		{
		for (i=0;i<aLen;i+=512)
			{
			TInt thisLen = (aLen-i) < 512 ? (aLen-i) : 512;
			TPtrC8 sectorWr(wrBuf.Mid(i, thisLen).Ptr(), thisLen);
			test(TheMmcDrive.Write((startSecPos+aFirstSectorOffset+i), sectorWr)==KErrNone);
			}
		}

	rdBuf.Fill(0,aLen);
	rdBuf.SetLength(aLen);

	if(aRdMB)
		{
		test(TheMmcDrive.Read((startSecPos+aFirstSectorOffset),aLen,rdBuf) == KErrNone);
		}
	else
		{
		for (i=0;i<aLen;i+=512)
			{
			TInt thisLen = (aLen-i) < 512 ? (aLen-i) : 512;
			TPtr8 sectorRd(((TUint8*)(rdBuf.Ptr()))+i, thisLen, thisLen);
			test(TheMmcDrive.Read((startSecPos+aFirstSectorOffset+i), thisLen, sectorRd) == KErrNone);
			}
		}

	test(CompareBuffers(rdBuf, wrBuf));

	// Check rest of first sector involved is unchanged (if offset specified)
	if (aFirstSectorOffset!=0)
		{
		rdBuf.Fill(0,KDiskSectorSize);
		test(TheMmcDrive.Read(startSecPos,KDiskSectorSize,rdBuf)==KErrNone);
		wrBuf.SetLength(KDiskSectorSize-aFirstSectorOffset);
		saveBuf1.Replace(aFirstSectorOffset,(KDiskSectorSize-aFirstSectorOffset),wrBuf);
		test(rdBuf.Compare(saveBuf1)==0);
		}

	// Check rest of last sector involved is unchanged (if not ending on sector boundary)
	if (endOffset)
		{
		rdBuf.Fill(0,KDiskSectorSize);
		test(TheMmcDrive.Read(endSecPos,KDiskSectorSize,rdBuf)==KErrNone);
		wrBuf.SetLength(aLen);
		wrBuf.Delete(0,aLen-endOffset);
		saveBuf2.Replace(0,endOffset,wrBuf);
		test(CompareBuffers(rdBuf, saveBuf2));
		}
	}

void MultipleSectorRdWrTest(TInt aFirstSectorOffset,TInt aLen, TBool aMBOnly = EFalse)
//
// Perform a write / read test over multiple sectors (starting within sector KMultSectorNo).
// Verify that the write / read back is successful and that the remainder of the first and
// last sectors are not affected.
//
	{
	test.Start(_L("Multiple sector write/read test"));

	if(!aMBOnly)
		{
		MultipleSectorRdWrTestMB(aFirstSectorOffset, aLen, EFalse, EFalse);
		MultipleSectorRdWrTestMB(aFirstSectorOffset, aLen, EFalse, ETrue);
		MultipleSectorRdWrTestMB(aFirstSectorOffset, aLen, ETrue,  EFalse);
		}

	MultipleSectorRdWrTestMB(aFirstSectorOffset, aLen, ETrue,  ETrue);

	test.End();
	}

LOCAL_C TInt dontDisconnectThread(TAny*)
	{

	TBusLocalDrive anotherMmcDrive;
	nTest.Title();

	nTest.Start(_L("Connect to internal drive"));
	anotherMmcDrive.Connect(DriveNumber,SecThreadChangeFlag);

	nTest.Next(_L("Capabilities"));
	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	nTest(anotherMmcDrive.Caps(infoPckg)==KErrNone);
	nTest(info.iType==EMediaHardDisk);

	nTest.End();
	return(KErrNone);
	}

LOCAL_C void ProgressBar(TInt64 aPos,TInt64 anEndPos,TInt anXPos)
//
// Display progress of local drive operation on screen (1-16 dots)
//
	{
	static TInt64 prev;
	TInt64 curr;
	if ((curr=(aPos-1)/(anEndPos>>4))>prev)
		{ // Update progress bar
		test.Console()->SetPos(anXPos);
		for (TInt64 i=curr;i>=0;i--)
			test.Printf(_L("."));
		}
	prev=curr;
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0510
@SYMTestCaseDesc Test Write/Read during media Change
@SYMTestPriority High

@SYMTestActions
		a.) Test Read during a Media Change
		b.) Test Write during a Media Change

@SYMTestExpectedResults All tests must pass
*/
LOCAL_C void TestHugeReadWrite(TBool aIsRead, TInt aLen)
//
// Writes aLen bytes to the MMC drive.  Gives user enough time to flip the media
// change switch.  Request should abort with KErrNotReady on write command, but nothing
// on read command.
// Each read or write is started from sector KMultSectNo (2).
// The media change operation only works when the switch is moved from the closed position
// to the open position.
// 
	{
	test.Start(_L("TestHugeReadWrite: media change during I/O test."));
	test.Printf(_L("aIsRead = %x, aLen = %x.\n"), aIsRead, aLen);

	HBufC8 *buf = HBufC8::New(aLen);
	test(buf != NULL);

	TInt startSectPos = KMultSectorNo << KDiskSectorShift;
	if (aIsRead)
		{
		test.Printf(_L("Launching %08x byte read at %08x.\n"), aLen, startSectPos);
		test.Printf(_L("Move media change from closed to open position before finished.\n"));
		TPtr8 ptr(buf->Des());
		TInt r = TheMmcDrive.Read(startSectPos, aLen, ptr);
		test.Printf(_L("r = %d.\n"), r);
		test(r == KErrNone);
		}
	else
		{
		buf->Des().Fill(0xff, aLen);
		test.Printf(_L("Launching %08x byte write at %08x.\n"), aLen, startSectPos);
		test.Printf(_L("Move media change from closed to open position before finished.\n"));
		TInt r = TheMmcDrive.Write(startSectPos, *buf);
		test.Printf(_L("r = %d.\n"), r);
		test(r == KErrNotReady);
		}
	
	test.Printf(_L("Pausing for 5 seconds to move media change switch back to closed.\n"));
	User::After(5 * 1000 * 1000);
	delete buf;
	test.End();
	}


LOCAL_C void FillBufferWithPattern(TDes8 &aBuf)
//
// Fills aBuf with cycling hex digits up to aBuf.Length().
//
	{
	TInt len = aBuf.Length() & ~3;
	for (TInt i = 0; i < len; i+=4)
		{
		*((TUint32*) &aBuf[i]) = i;
		}
	}


LOCAL_C void WriteAndReadBack(TInt64 aStartPos, const TDesC8 &aWrBuf)
//
// This function tests the multiple block reads when aWrBuf is sufficiently large.
//
	{
	test.Start(_L("WriteAndReadBack"));

	TInt r;										// general error values

	// Allocate a same size buffer to read back into and compare with.
	HBufC8 *rdBuf = aWrBuf.Alloc();
	test(rdBuf != NULL);
	TPtr8 rdPtr(rdBuf->Des());
	
	test.Next(_L("wrb: writing"));
	r = TheMmcDrive.Write(aStartPos, aWrBuf);
	test.Printf(_L("\nwrb:r=%d"), r);
	test(r == KErrNone);

	test.Printf(_L("\n"));
	test.Next(_L("wrb: reading"));
	r = TheMmcDrive.Read(aStartPos, rdPtr.Length(), rdPtr);
	test.Printf(_L("rb:r=%d"), r);
	test(r == KErrNone);

	// Compare the pattern that has just been read back with the original.
	test.Printf(_L("\n"));
	test.Next(_L("wrb: comparing"));
	test.Printf(
		_L("rdPtr.Length() = %04x, aWrBuf.Length() = %04x"),
		rdPtr.Length(), aWrBuf.Length());
	test(rdPtr == aWrBuf);

#if 0											// extra debug when buffers not compare.
	for (TInt j = 0; j < rdPtr.Length(); j++)
		{
		test.Printf(_L("%d: w%02x r%02x"), j, aWrBuf[j], rdBuf[j]);

		if (rdPtr[j] != aWrBuf[j])
			{
			test.Printf(_L("buffer mismatch at %04x: %02x v %02x"), j, rdPtr[j], aWrBuf[j]);
			test(EFalse);
			}
		}
#endif

	test.Printf(_L("\n"));
	delete rdBuf;
	test.End();
	}

/**
@SYMTestCaseID PBASE-T_MMCDRV-0169
@SYMTestCaseDesc Test Multiple Block Reads
@SYMTestPriority High

@SYMTestActions
		a.) Test Multiple Block Reads at the internal buffer size
		b.) Test Multiple Block Reads greater than the internal buffer size

@SYMTestExpectedResults All tests must pass

@TODO: increase Buffer size to match current reference platform (128KB)
*/
LOCAL_C void TestMultipleBlockReads()
	{
	// Test multiple block reads.
	static TBuf8<256 * 1024> rw_wrBuf;

	rw_wrBuf.SetLength(rw_wrBuf.MaxLength());
	FillBufferWithPattern(rw_wrBuf);

	test.Next(_L("Testing multiple block reads at internal buffer size"));
	rw_wrBuf.SetLength(8 * KDiskSectorSize);
	WriteAndReadBack(KMultSectorNo << KDiskSectorShift, rw_wrBuf);

	test.Next(_L("Testing multiple block reads at gt internal buffer size"));
	rw_wrBuf.SetLength(10 * KDiskSectorSize);
	WriteAndReadBack(KMultSectorNo << KDiskSectorShift, rw_wrBuf);

	test.Next(_L("Testing unaligned large block read "));
	rw_wrBuf.SetLength(rw_wrBuf.MaxLength());
	WriteAndReadBack((KMultSectorNo << KDiskSectorShift) + 128, rw_wrBuf);
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0558
@SYMTestCaseDesc Test Long Read/Write Boundaries
@SYMTestPriority High

@SYMTestActions  
	
  Perform and Write/Read/Verify for the given length (L) of data across the following boundaries.
  Depending on the length provided, this will also perform a partial write/read at the end sector.

									 -------------------
									| Start	|	End		|
									|-------------------|
									| 0		|	L		|
									| 507	|	L-507	|
									| 10	|	L		|
									| 0		|	L-3		|
									| 27	|	L-512	|
									| 0		|	L-509	|
									| 3		|	L-3		|
									 -------------------

  For each combination, the write/read/verify operations are performed in the following sequence:

	a: Write and Read in single 512-byte blocks.
	b: Write in a single operation (multiple blocks), Read in 512-Byte blocks.
	c: Write in 512-Byte blocks, Read in a single operation (multiple-blocks).
	d: Write and Read in a single operation (multiple-blocks).

  In the cases where a partial read/write operation occurs (ie - the start and/or end position don't lie within
  a sector boundary), the original contents of the start and/or end sectors are read and stored at the start of
  the test, and compared with the contents of the sectors at the end of the test to ensure that unwritten data within
  the sectors remain unaffected.
  
@SYMTestExpectedResults All tests must pass

@SYMPREQ1389 REQ6951 Double Buffering and SD Switch
*/
	
LOCAL_C void TestLongReadWriteBoundaries(TUint aLen, TBool aMBOnly = EFalse)
	{
	TBuf<64> b;

	b.Format(_L("MMC drive: Very long RdWr(1) (%dbytes at %d)"),aLen,0);
	test.Next(b);
	MultipleSectorRdWrTest(0, aLen, aMBOnly); // Exceeds driver's buffer, starts/ends on sector boundary

	b.Format(_L("MMC drive: Very long RdWr(2) (%dbytes at %d)"),(aLen-KDiskSectorSize+5),507);
	test.Next(b);
	MultipleSectorRdWrTest(507, (aLen-KDiskSectorSize+5), aMBOnly); // Exceeds driver's buffer, ends on sector boundary

	b.Format(_L("MMC drive: Very long RdWr(3) (%dbytes at %d)"),aLen,10);
	test.Next(b);
	MultipleSectorRdWrTest(10, aLen, aMBOnly); // Exceeds driver's buffer, starts/ends off sector boundary

	b.Format(_L("MMC drive: Very long RdWr(4) (%dbytes at %d)"),(aLen-3),0);
	test.Next(b);
	MultipleSectorRdWrTest(0, aLen-3, aMBOnly); // Exceeds driver's buffer, starts on sector boundary

	b.Format(_L("MMC drive: Very long RdWr(5) (%dbytes at %d)"),(aLen-KDiskSectorSize),27);
	test.Next(b);
	MultipleSectorRdWrTest(27, (aLen-KDiskSectorSize), aMBOnly); // Exceeds driver's buffer (due to start offset), starts/ends off sector boundary

	b.Format(_L("MMC drive: Very long RdWr(6) (%dbytes at %d)"),(aLen-KDiskSectorSize-3),0);
	test.Next(b);
	MultipleSectorRdWrTest(0, aLen-KDiskSectorSize-3, aMBOnly); // Equals driver's buffer, starts on sector boundary

	b.Format(_L("MMC drive: Very long RdWr(7) (%dbytes at %d)"),(aLen-3),3);
	test.Next(b);
	MultipleSectorRdWrTest(3, aLen-3, aMBOnly); // Equals driver's buffer, ends on sector boundary
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0509
@SYMTestCaseDesc Test Sector Read/Writing
@SYMTestPriority High

@SYMTestActions
		a.) Test Writing blocks on sector boundaries
		b.) Test Reading blocks on sector boundaries
		c.) Test single sector Write/Read at:
			  i.) Sector Start
			 ii.) Mid Sector
			iii.) Sector End
		d.) Test Multiple Sector Write/Read:
			  i.) Start on Sector Boundary
			 ii.) Start/End on Sector Boundary
			iii.) End on Sector Boundary
		e.) Test Write/Read over sector boundary

@SYMTestExpectedResults All tests must pass
*/
LOCAL_C void TestSectorReadWrite()
	{
	TBuf<64> b;
	b.Format(_L("MMC drive: Sector RdWr(%d)"), KDiskSectorSize);

	test.Next(b);

	TInt len;

	// Fill wrBuf with a pattern of ascending numbers.
	wrBuf.SetLength(KDiskSectorSize);
	TUint32 *p = REINTERPRET_CAST(TUint32 *, &wrBuf[0]);
	TInt secPos;
	for (secPos = 0; secPos < KDiskSectorSize; secPos++)
		{
		wrBuf[secPos] = TUint8(secPos % 0x0100);
		}

	// Write 512 byte blocks to the card, writing the sector number to the first
	// word in each buffer.

	test.Printf(_L("Writing    "));
	TInt64 i;
//	for (i=0;i<DriveSize;i+=len)  // B - Sector wr/rd on sector boundary
	for (i=0;i<(0x200<<3);i+=len)	 // B - Sector wr/rd on sector boundary
		{
		ProgressBar(i, TheMmcDrive.Size(), 11);
		len = KDiskSectorSize < TheMmcDrive.Size() - i ? KDiskSectorSize : I64LOW(TheMmcDrive.Size() - i);
		(*p) = I64LOW(i) / KDiskSectorSize;
		wrBuf.SetLength(len);
		TInt r = TheMmcDrive.Write(i, wrBuf);
		if (r != KErrNone)
			{
			test.Printf(_L("wt:i = %d, len = %d, r  %d"), i, len, r);
			test(EFalse);
			}
		}

	// Read each of the 512 byte blocks back from the card.
	test.Printf(_L("\r\nReading    "));
//	for (i=0;i<TheMmcDrive.Size();i+=len)
	for (i=0;i<(0x200<<3);i+=len)	 // B - Sector wr/rd on sector boundary
		{
		ProgressBar(i, TheMmcDrive.Size(), 11);
		len = KDiskSectorSize < TheMmcDrive.Size() - i ? KDiskSectorSize : I64LOW(TheMmcDrive.Size() - i);
		rdBuf.Fill(0,len);
		TInt r = TheMmcDrive.Read(i, len, rdBuf);
		if (r != KErrNone)
			{
			test.Printf(_L("rd:i = %d, len = %d, r  %d"), i, len, r);
			test(EFalse);
			}
		(*p) = (I64LOW(i)/KDiskSectorSize);
		wrBuf.SetLength(len);

		if ((r = rdBuf.Compare(wrBuf)) != 0)
			{
			test.Printf(_L("wc:i = %d, len = %d, r  %d"), i, len, r);
			test.Printf(_L("wc: wrBuf.Length() = %d, rdBuf.Length() = %d"), wrBuf.Length(), rdBuf.Length());
			TInt j;
			for (j = 0; j < wrBuf.Length() && wrBuf[j] == rdBuf[j]; j++)
				{
				// empty.
				}
			test.Printf(_L("wc: wrBuf[%d] = %d, rdBuf[%d] = %d"), j, wrBuf[j], j, rdBuf[j]);

			test(EFalse);
			}
		}
	test.Printf(_L("\r\n"));

	b.Format(_L("MMC drive: Short RdWr(1) (%dbytes at %d)"),25,0); 
	test.Next(b);
	singleSectorRdWrTest(0,25); // A - Sub-sector wr/rd at sector start

	b.Format(_L("MMC drive: Short RdWr(2) (%dbytes at %d)"),16,277); 
	test.Next(b);
	singleSectorRdWrTest(277,16); // E - Sub-sector wr/rd in mid sector

	b.Format(_L("MMC drive: Short RdWr(3) (%dbytes at %d)"),100,412); 
	test.Next(b);
	singleSectorRdWrTest(412,100); // F - Sub-sector wr/rd at sector end

	b.Format(_L("MMC drive: Long RdWr(1) (%dbytes at %d)"),KDiskSectorSize+15,0);
	test.Next(b);
	MultipleSectorRdWrTest(0,KDiskSectorSize+15); // C - Long wr/rd starting on sector boundary

	b.Format(_L("MMC drive: Long RdWr(2) (%dbytes at %d)"),(KDiskSectorSize<<1),0);
	test.Next(b);
	MultipleSectorRdWrTest(0,(KDiskSectorSize<<1)); // D - Long wr/rd starting/ending on sector boundary

	b.Format(_L("MMC drive: Long RdWr(3) (%dbytes at %d)"),KDiskSectorSize+3,509);
	test.Next(b);
	MultipleSectorRdWrTest(509,KDiskSectorSize+3); // H -  - Long wr/rd ending on sector boundary

	b.Format(_L("MMC drive: Long RdWr(4) (%dbytes at %d)"),(KDiskSectorSize<<1),508);
	test.Next(b);
	MultipleSectorRdWrTest(508,(KDiskSectorSize<<1));

	b.Format(_L("MMC drive: Sector RdWr across sector boundary(%dbytes at %d)"),KDiskSectorSize,508);
	test.Next(b);
	MultipleSectorRdWrTest(508,KDiskSectorSize);	// G - Sector wr/rd over sector boundary

	TestLongReadWriteBoundaries(KRdWrBufLen);			// Short length - As per original test

	if (ManualMode)
		{
		for(TInt bufLen = KRdWrBufLen; bufLen <= 256*1024; bufLen += KRdWrBufLen)
			{
			TestLongReadWriteBoundaries(bufLen, ETrue);				// Very long length - to test Double-Buffering
			}
		
		TestLongReadWriteBoundaries(KVeryLongRdWrBufLen, ETrue);	// Very long length - to test Double-Buffering
		}
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0168
@SYMTestCaseDesc Test Sector Formatting
@SYMTestPriority High

@SYMTestActions
		a.) Test Format/Read/Verify Single Sector
		b.) Test Format/Read/Verify Multiple Sectors
		c.) Test Format/Read/Verify Whole Media

@SYMTestExpectedResults All tests must pass
*/
LOCAL_C void TestFormat()
	{
	if(TheMmcDrive.TestMode() != TMMCDrive::ETestPartition)
		{
		test.Printf(_L("Skipping format tests - only supported on Partition Test Mode"));
		return;
		}

	if(CardType == TKnownCardTypes::EBuffalloMiniSD_32M_ERASE ||	
	   CardType == TKnownCardTypes::EBuffalloMiniSD_64M_ERASE ||
	   CardType == TKnownCardTypes::EBuffalloMiniSD_128M_ERASE ||
	   CardType == TKnownCardTypes::EBuffalloMiniSD_256M_ERASE ||
	   CardType == TKnownCardTypes::EBuffalloMiniSD_512M_ERASE
	   )
	    {
	    //These cards implement the erase command incorrectly
	    test.Printf( _L(" -- Skipping Format Tests - Known card detected --\n") );
	    return;
	    }
	
	test.Next(_L("MMC drive: Format sectors (short)"));
	TBuf8<KDiskSectorSize> savBuf1,savBuf2;
	TInt fmtTestPos=(10<<KDiskSectorShift);
	// Save sectors surrounding those which will be formatted
	test(TheMmcDrive.Read((fmtTestPos-KDiskSectorSize),KDiskSectorSize,savBuf1)==KErrNone);
	test(TheMmcDrive.Read((fmtTestPos+KShortFormatInBytes),KDiskSectorSize,savBuf2)==KErrNone);

	// Fill buffer with 0xCC 
	// (i.e. a value which is not going to be written by formatting the device)
	// & then write to area which is to be formatted
	wrBuf.SetLength(KShortFormatInBytes);
	wrBuf.Fill(0xCC);
	test(TheMmcDrive.Write(fmtTestPos, wrBuf)==KErrNone);


	test(TheMmcDrive.Format(fmtTestPos,KShortFormatInBytes)==KErrNone);
	test(TheMmcDrive.Read(fmtTestPos,KShortFormatInBytes,rdBuf)==KErrNone);

	TUint8 defEraseVal = rdBuf[0];
	test(defEraseVal == 0x00 || defEraseVal == 0xFF);	// The card should erase with 0x00 or 0xFF
	wrBuf.Fill(defEraseVal ,KShortFormatInBytes);
	test(rdBuf.Compare(wrBuf)==0);

	// Check that surrounding sectors unaffected
	test(TheMmcDrive.Read((fmtTestPos-KDiskSectorSize),KDiskSectorSize,rdBuf)==KErrNone);
	test(rdBuf.Compare(savBuf1)==0);
	test(TheMmcDrive.Read((fmtTestPos+KShortFormatInBytes),KDiskSectorSize,rdBuf)==KErrNone);
	test(rdBuf.Compare(savBuf2)==0);

	test.Next(_L("MMC drive: Format sectors (long)"));
	fmtTestPos+=(4<<KDiskSectorShift);
	// Save sectors surrounding those which will be formatted
	test(TheMmcDrive.Read((fmtTestPos-KDiskSectorSize),KDiskSectorSize,savBuf1)==KErrNone);
	test(TheMmcDrive.Read((fmtTestPos+KLongFormatInBytes),KDiskSectorSize,savBuf2)==KErrNone);

	// Fill buffer with 0xCC 
	// (i.e. a value which is not going to be written by formatting the device)
	// & then write to area which is to be formatted
	wrBuf.SetLength(KLongFormatInBytes);
	wrBuf.Fill(0xCC);
	test(TheMmcDrive.Write(fmtTestPos, wrBuf)==KErrNone);

	test(TheMmcDrive.Format(fmtTestPos,KLongFormatInBytes)==KErrNone);
	test(TheMmcDrive.Read(fmtTestPos,KLongFormatInBytes,rdBuf)==KErrNone);

	defEraseVal = rdBuf[0];
	test(defEraseVal == 0x00 || defEraseVal == 0xFF);	// The card should erase with 0x00 or 0xFF
	wrBuf.Fill(defEraseVal,KLongFormatInBytes);
	TInt cmpRes = rdBuf.Compare(wrBuf);
	if(cmpRes != 0)
		{
		test.Printf(_L("\n\rExpected 0x%02x\n\r"));
		for(TInt x=0; x<KLongFormatInBytes; x+=8)
			{
			test.Printf(_L("%08x : %02x %02x %02x %02x %02x %02x %02x %02x\n\r"), x, rdBuf[x],rdBuf[x+1],rdBuf[x+2],rdBuf[x+3],rdBuf[x+4],rdBuf[x+5],rdBuf[x+6],rdBuf[x+7]);
			}
		}
	test(cmpRes==0);

	// Check that surrounding sectors unaffected
	test(TheMmcDrive.Read((fmtTestPos-KDiskSectorSize),KDiskSectorSize,rdBuf)==KErrNone);
	test(rdBuf.Compare(savBuf1)==0);
	test(TheMmcDrive.Read((fmtTestPos+KLongFormatInBytes),KDiskSectorSize,rdBuf)==KErrNone);
	test(rdBuf.Compare(savBuf2)==0);

	if (ManualMode)
		{
		test.Next(_L("Fill the drive with garbage"));
		TInt64 driveSize = TheMmcDrive.Size();
		TInt wtLen = wrBuf.MaxLength();
		TInt64 i;
		for (i=0; i<driveSize; i+=wtLen)
			{
			ProgressBar(i,driveSize,11);
			wtLen = wtLen < driveSize - i ? wtLen : I64LOW(driveSize - i);
			wrBuf.Fill(0xCC,wtLen);

			wrBuf.SetLength(wtLen);

			test.Printf(_L("writing pos %08lX len %08X\n"), i, wrBuf.Length());
			test(TheMmcDrive.Write(i, wrBuf) == KErrNone);
			}

		test.Next(_L("MMC drive: Format entire disk"));
		TFormatInfo fi;
		test.Printf(_L("Formatting "));
		TInt ret;
		TInt stage = 0;
		while((ret=TheMmcDrive.Format(fi))!=KErrEof)
			{
			stage++;
			ProgressBar((fi.i512ByteSectorsFormatted<<9),TheMmcDrive.Size(),11);
			test(ret==KErrNone);
			}

		test.Printf(_L("\r\nReading    "));
		
		TInt len = KVeryLongSectBufSizeInBytes;

		for (i=0; i<TheMmcDrive.Size(); i+=len)
			{
			ProgressBar(i,TheMmcDrive.Size(),11);
			len = len < TheMmcDrive.Size() - i ? len : I64LOW(TheMmcDrive.Size() - i);
			rdBuf.Fill(0x55,len);
			test(TheMmcDrive.Read(i,len,rdBuf) == KErrNone);

			const TInt wholeSectors = len / KDiskSectorSize;
			const TInt rem = len - (wholeSectors * KDiskSectorSize);

			TInt sec;
			for(sec=1;sec<wholeSectors; sec++)	// Start at Base+1 - Card may have written an MBR at sector 0
				{
				wrBuf.SetLength(KDiskSectorSize);
				defEraseVal = rdBuf[sec * KDiskSectorSize];
				test(defEraseVal == 0x00 || defEraseVal == 0xFF);	// The card should erase with 0x00 or 0xFF
				wrBuf.Fill(defEraseVal, KDiskSectorSize);
				test( CompareBuffers( wrBuf, rdBuf.Mid( sec * KDiskSectorSize, KDiskSectorSize ) ) );
				}

			if(rem > 0)
				{
				wrBuf.SetLength(rem);
				defEraseVal = rdBuf[sec * KDiskSectorSize];
				test(defEraseVal == 0x00 || defEraseVal == 0xFF);	// The card should erase with 0x00 or 0xFF
				wrBuf.Fill(defEraseVal, rem);
				test( CompareBuffers( wrBuf, rdBuf.Mid( sec * KDiskSectorSize, rem ) ) );
				}
			}
		}
	}


class TRandGen
	{
	public:
		TRandGen();
		void Seed();
		void Seed( const TInt64& aSeed );
		TUint Next();

	private:
		TInt64	iValue;
	};


TRandGen::TRandGen()
	: iValue(KDefaultRandSeed)
	{
	}


void TRandGen::Seed( const TInt64& aSeed )
	{
	iValue = aSeed;
	}

void TRandGen::Seed()
	{
	iValue = KDefaultRandSeed;
	}

TUint TRandGen::Next()
	{
	iValue *= 214013;
    iValue += 2531011;
    return static_cast<TUint>( I64LOW(iValue) );
	}


GLDEF_C void FillRandomBuffer( TDes8& aBuf, TRandGen& aRand )
	/**
	 * Fill buffer aBuf with data generated by aRand
	 */
	{
	TUint l = aBuf.MaxLength();
	aBuf.SetLength( l );
	TUint* p = (TUint*)aBuf.Ptr();

	// Do any unaligned bytes at the start
	TInt preAlign = (TUint)p & 3;
	if( preAlign )
		{
		preAlign = 4 - preAlign;
		TUint8* p8 = (TUint8*)p;
		TUint rand = aRand.Next();
		while( preAlign && l )
			{
			*p8 = (TUint8)(rand & 0xFF);
			rand >>= 8;
			++p8;
			--preAlign;
			--l;
			}
		p = (TUint*)p8;
		}

	for( ; l > 3; l-=4 )
		{
		*p++ = aRand.Next();
		}
	// Fill in any trailing bytes
	if( l > 0 )
		{
		TUint8* q = (TUint8*)p;
		TUint r = aRand.Next();
		if( l > 1 )
			{
			*((TUint16*)q) = (TUint16)(r & 0xFFFF);
			q += 2;
			l -= 2;
			r >>= 16;
			}
		if( l > 0 )
			{
			*q = (TUint8)(r & 0xFF);
			}
		}
	}

GLDEF_C void FillRandomBuffer( HBufC8* aBuf, TRandGen& aRand )
	/**
	 * Fill buffer aBuf with data generated by aRand
	 * For convenience this version takes a HBufC8*
	 */
	{
	TPtr8 ptr = aBuf->Des();
	FillRandomBuffer( ptr, aRand );
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0164
@SYMTestCaseDesc Test MMC Drive Capabilities
@SYMTestPriority High

@SYMTestActions  
	a. Obtain MMC Drive Capabilities
	b. If the card size is greater than 2GBytes, test that the driver reports FAT32 file system supported.
	c. Test that the type of media is reported as EMediaHardDisk
	d. Test that the drive attributes report KDriveAttLocal and KDriveAttRemovable
	e. Test that the drive attributes do not report KDriveAttRemote
	f. If the drive is not write protected or a ROM card, test that the media attributes report that the drive is formattable
	g. If the drive is write protected or a ROM card, test that the media attributes do not report that the drive is formattable
	h. Test that the media attributes do not report variable sized media.

@SYMTestExpectedResults All tests must pass

@SYMPREQ1389 CR0795 Support for >2G SD Cards
*/
TBool TestDriveInfo()
	{
	test.Next( _L("Test drive info") );

	TEST_FOR_ERROR( TheMmcDrive.Caps( DriveCaps ) );

	test.Printf( _L("Caps V1:\n\tiSize=0x%lx\n\tiType=%d\n\tiConnectionBusType=%d\n\tiDriveAtt=0x%x\n\tiMediaAtt=0x%x\n\tiBaseAddress=0x%x\n\tiFileSystemId=0x%x\n\tiPartitionType=0x%x\n"),
			DriveCaps().iSize,
			DriveCaps().iType,
			DriveCaps().iConnectionBusType,
			DriveCaps().iDriveAtt,
			DriveCaps().iMediaAtt,
			DriveCaps().iBaseAddress,
			DriveCaps().iFileSystemId,
			DriveCaps().iPartitionType );

	test.Printf( _L("Caps V2:\n\tiHiddenSectors=0x%x\n\tiEraseBlockSize=0x%x\nCaps V3:\n\tiExtraInfo=%x\n\tiMaxBytesPerFormat=0x%x\n"),
			DriveCaps().iHiddenSectors,
			DriveCaps().iEraseBlockSize, 
			DriveCaps().iExtraInfo,
			DriveCaps().iMaxBytesPerFormat );

	test.Printf( _L("Format info:\n\tiCapacity=0x%lx\n\tiSectorsPerCluster=0x%x\n\tiSectorsPerTrack=0x%x\n\tiNumberOfSides=0x%x\n\tiFatBits=%d\n"),
			DriveCaps().iFormatInfo.iCapacity,
			DriveCaps().iFormatInfo.iSectorsPerCluster,
			DriveCaps().iFormatInfo.iSectorsPerTrack,
			DriveCaps().iFormatInfo.iNumberOfSides,
			DriveCaps().iFormatInfo.iFATBits );

	if(DriveCaps().iSerialNumLength > 0)
		{
        test.Printf( _L("Serial Number : ") );
        TBuf8<2*KMaxSerialNumLength> snBuf;
        TUint i;
		for (i=0; i<DriveCaps().iSerialNumLength; i++)
			{
            snBuf.AppendNumFixedWidth( DriveCaps().iSerialNum[i], EHex, 2 );
			test.Printf( _L("%02x"), DriveCaps().iSerialNum[i]);
			}
		test.Printf( _L("\n") );

		CardType = TKnownCardTypes::EStandardCard;
		for(i=0; i < sizeof(KnownCardTypes) / sizeof(TKnownCardTypes); i++)
			{
			TPtrC8 serial(KnownCardTypes[i].iSerialNumber);
			if(snBuf.Compare(serial) == 0)
				{
				CardType = KnownCardTypes[i].iCardType;
				break;
				}
			}
		}
	else
		{
		test.Printf( _L("Serial Number : Not Supported") );
		}

	// DriveSize - The size of the partition to which the test is connected.
	// MediaSize - The entire size of the media containing the partition.
	
	TInt64 mediaSize = DriveCaps().MediaSizeInBytes();
	TheMmcDrive.SetSize(DriveCaps().iSize, mediaSize);
	if(mediaSize == 0)
		{
		test.Printf(_L("Check entire media size: Not Supported\r\n"));
		}

	test.Printf(_L("Entire media size: %ld\r\n"),mediaSize);
	test.Printf(_L("Partition size:    %ld\r\n"),DriveCaps().iSize);
	test.Printf(_L("Hidden sectors:    %d\r\n"),DriveCaps().iHiddenSectors);
	
	
	TEST_FOR_VALUE( DriveCaps().iFileSystemId, KDriveFileSysFAT );
	
	// Test that a drive >2GB is marked as requesting FAT32
	if( DriveCaps().iSize > KTwoGigbytes && DriveCaps().iExtraInfo)
		{
		TEST_FOR_VALUE( DriveCaps().iFormatInfo.iFATBits, TLDFormatInfo::EFB32 );
		}

	TEST_FOR_VALUE( DriveCaps().iType, EMediaHardDisk );
	
	const TUint KExpectedDriveAtt = KDriveAttLocal | KDriveAttRemovable;
	const TUint KNotExpectedDriveAtt = KDriveAttRemote;
	TEST_FOR_VALUE( DriveCaps().iDriveAtt & KExpectedDriveAtt, KExpectedDriveAtt );
	TEST_FOR_VALUE( DriveCaps().iDriveAtt & KNotExpectedDriveAtt, 0 );

	TUint expectedMediaAtt = KMediaAttFormattable;
	TUint notExpectedMediaAtt = KMediaAttVariableSize;

	TBool isReadOnly = DriveCaps().iMediaAtt & KMediaAttWriteProtected;
	if(isReadOnly)
		{
		expectedMediaAtt &= ~KMediaAttFormattable;

		test.Printf( _L("\n ---------------------------\n") );
		test.Printf( _L("  Media is Write Protected\n") );
		if((DriveCaps().iMediaAtt & KMediaAttFormattable) != KMediaAttFormattable)
			{
			test.Printf( _L("    Media is a ROM card\n") );
			}
		test.Printf( _L("  Some tests will be skipped\n") );
		test.Printf( _L(" ---------------------------\n") );
		}

	TEST_FOR_VALUE( DriveCaps().iMediaAtt & expectedMediaAtt, expectedMediaAtt );
	TEST_FOR_VALUE( DriveCaps().iMediaAtt & notExpectedMediaAtt, 0 );

	return(isReadOnly);
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0165
@SYMTestCaseDesc Test MMC Card Reads
@SYMTestPriority High

@SYMTestActions  
	a. Read 64K in one operation from the start of the media and store the contents.
	b. Read 512 byte blocks from the start of the media at various offsets and compare with initial read.
	b. Read 64K in 512 byte blocks from the start of the media and compare with the initial read.
	c. read 64K from the end of the drive

@SYMTestExpectedResults All tests must pass

@SYMPREQ1389 CR0795 Support for >2G SD Cards
*/
void TestRead()
	{
	// This just tests that we can read *something* from the drive
	// We check elsewhere that we can read what we've written
	test.Next( _L("Test reading" ) );

	HBufC8* bigBuf = HBufC8::New( 65536 );
	HBufC8* smallBuf = HBufC8::New( 512 );

	test( bigBuf != NULL );
	test( smallBuf != NULL );
	TPtr8 bigPtr( bigBuf->Des() );
	TPtr8 smallPtr( smallBuf->Des() );

	test.Printf( _L("Read block from start of media\n") );
	TEST_FOR_ERROR( TheMmcDrive.Read( TInt64(0), 65536, bigPtr) );

	test.Printf( _L("Read smaller blocks which should match the data in big block\n\r" ) );
	TInt i;
	for( i = 0; i <= 512; ++i )
		{
		test.Printf( _L("\toffset: %d\r"), i );
		TEST_FOR_ERROR( TheMmcDrive.Read( TInt64(i), 512, smallPtr ) );
		test( CompareBuffers( smallBuf->Des(), bigBuf->Mid( i, 512 ) ) );
		}

	for( i = 512; i <= 65536-512; i += 512 )
		{
		test.Printf( _L("\toffset: %d\r"), i );
		TEST_FOR_ERROR( TheMmcDrive.Read( TInt64(i), 512, smallPtr ) );
		test( CompareBuffers( smallBuf->Des(), bigBuf->Mid( i, 512 ) ) );
		}

	test.Printf( _L("\nTest read from end of drive\n") );
	
	if(CardType == TKnownCardTypes::EBuffalloMiniSD_512M ||	
	   CardType == TKnownCardTypes::EIntegralHSSD_2G)
		{
		// These cards have issues with reading at the end of the drive...
		test.Printf( _L(" -- Skipping Test - Known card detected --\n") );
		}
	else
		{
		TEST_FOR_ERROR( TheMmcDrive.Read( TheMmcDrive.Size() - 65536, 65536, bigPtr) );
		}

	delete smallBuf;
	delete bigBuf;
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0511
@SYMTestCaseDesc Test Moving Read/Write
@SYMTestPriority High

@SYMTestActions
		a.) Test Read/Verify Whole Sectors
		b.) Test Read/Verify Sliding sector sized window
		c.) Test Read/Verify Sliding byte sized window
		d.) Test Read/Verify Increasing sized window
		e.) Test Write/Read/Verify Whole Sectors
		f.) Test Write/Read/Verify Sliding sector sized window
		g.) Test Write/Read/Verify Increasing sized window
		
@SYMTestExpectedResults All tests must pass
*/
void DoReadWriteTest( TInt64 aPos, TInt aWindowSize, TBool aQuick )
	{
	// Do various read/write tests within a aWindowSize window starting at aPos
	HBufC8* wholeBuf = HBufC8::New( aWindowSize );
	test( wholeBuf != NULL );

	HBufC8* readBuf = HBufC8::New( aWindowSize );
	test( readBuf != NULL );

	TBuf8<512> sectorBuf;
	TRandGen rand;
	
	test.Printf( _L("Walking sector read\n\r") );
	FillRandomBuffer( wholeBuf, rand );
	TPtr8 wholeBufPtr( wholeBuf->Des() );
	TEST_FOR_ERROR( TheMmcDrive.Write( aPos, *wholeBuf ) );
	
	// Read each sector back and check that it's correct
	TInt64 pos( aPos );
	TInt i;
	for( i = 0; i < aWindowSize - 512; i += 512 )
		{
		pos = aPos + i;
		test.Printf(_L("\tRead @0x%lx\r"), pos);
		TEST_FOR_ERROR( TheMmcDrive.Read( pos, 512, sectorBuf ) );
		test( CompareBuffers( sectorBuf, wholeBuf->Mid( i, 512 ) ) );
		}

	test.Printf( _L("\nSliding sector read\n\r") );
	// Slide a sector-sized window over the data
	TInt maxl = Min( aWindowSize - 512, 512 * 3 );
	for( i = 0; i < maxl; i++ )
		{
		pos = aPos + i;
		test.Printf(_L("\tRead @0x%lx\r"), pos);
		TEST_FOR_ERROR( TheMmcDrive.Read( pos, 512, sectorBuf ) );
		test( CompareBuffers( sectorBuf, wholeBuf->Mid( i, 512 ) ) );
		}
	
	if( !aQuick )
		{
		test.Printf( _L("\nSliding byte read\n\r") );
		// Slide a byte-sized window over the data
		for( i = 0; i < maxl; i++ )
			{
			pos = aPos + i;
			test.Printf(_L("\tRead @0x%lx\r"), pos);
			TEST_FOR_ERROR( TheMmcDrive.Read( pos, 1, sectorBuf ) );
			test( CompareBuffers( sectorBuf, wholeBuf->Mid( i, 1 ) ) );
			}

		test.Printf( _L("\nGrowing read\n\r") );
		// Read from an increasing-sized window
		for( i = 1; i < 512; i++ )
			{
			test.Printf(_L("\tRead length: %d\r"), i);
			TEST_FOR_ERROR( TheMmcDrive.Read( aPos, i, sectorBuf ) );
			test( CompareBuffers( sectorBuf, wholeBuf->Left( i ) ) );
			}

		test.Printf( _L("\nDownward-expanding read\n\r") );
		// Read from a window that grows downward from the end of the test region
		for( i = 1; i <= 512; i++ )
			{
			pos = aPos + aWindowSize - i;
			test.Printf(_L("\t[pos:len] %lx:%d\r"), pos, i);
			TEST_FOR_ERROR( TheMmcDrive.Read( pos, i, sectorBuf ) );
			test( CompareBuffers( sectorBuf, wholeBuf->Mid( aWindowSize - i, i ) ) );
			}
		}

	test.Printf( _L("\nWalking sector write\n\r") );
	// Overwrite each sector and check the whole region is correct
	for( i = 0; i < aWindowSize - 512; i += 512 )
		{
		FillRandomBuffer( sectorBuf, rand );
		pos = aPos + i;
		test.Printf(_L("\tWrite @0x%lx\r"), pos);
		TEST_FOR_ERROR( TheMmcDrive.Write( pos, sectorBuf ) );
		wholeBufPtr.MidTPtr( i, 512 ) = sectorBuf;	// update our match data
		
		TPtr8 ptr( readBuf->Des() );
		TEST_FOR_ERROR( TheMmcDrive.Read( aPos, aWindowSize, ptr ) );
		test( CompareBuffers( *readBuf, *wholeBuf ) );
		}

	if( !aQuick )
		{
		test.Printf( _L("\nSliding sector overwrite\n\r") );
		// Overwrite a sector-sized region that slides across the test region
		for( i = 0; i < maxl; i += 1 )
			{
			FillRandomBuffer( sectorBuf, rand );
			pos = aPos + i;
			test.Printf(_L("\tWrite @0x%lx\r"), pos);
			TEST_FOR_ERROR( TheMmcDrive.Write( pos, sectorBuf ) );
			wholeBufPtr.MidTPtr( i, 512 ) = sectorBuf;	// update our match data
			
			TPtr8 ptr( readBuf->Des() );
			TEST_FOR_ERROR( TheMmcDrive.Read( aPos, aWindowSize, ptr ) );
			test( CompareBuffers( *readBuf, *wholeBuf ) );
			}

		test.Printf( _L("\nGrowing sector overwrite\n\r") );
		// Overwrite an expanding region starting at aPos
		for( i = 1; i < 512; i += 1 )
			{
			FillRandomBuffer( sectorBuf, rand );
			test.Printf(_L("\tWrite length: %d\r"), i);
			sectorBuf.SetLength( i );
			TEST_FOR_ERROR( TheMmcDrive.Write( aPos, sectorBuf ) );
			wholeBufPtr.LeftTPtr( i ) = sectorBuf;	// update our match data
			
			TPtr8 ptr( readBuf->Des() );
			TEST_FOR_ERROR( TheMmcDrive.Read( aPos, aWindowSize, ptr ) );
			test( CompareBuffers( *readBuf, *wholeBuf ) );
			}
		}

	test.Printf( _L("\nTest zero-length read\n") );
	FillRandomBuffer( sectorBuf, rand );
	TEST_FOR_ERROR( TheMmcDrive.Read( aPos, 0, sectorBuf ) );
	TEST_FOR_VALUE( sectorBuf.Length(), 0 );

	delete wholeBuf;
	delete readBuf;
	}


// This tests for a bug observed in certain ESanDiskMmcMobile_1GB cards which never exit the busy state
// when writing a buffer which is one sector bigger than the PSL buffer size (resulting in a single write
// request split into 2 fragments, the last of which is one sector only). The "fix" for this is to make the 
// PSL reject CMD23 (SET_BLOCK_COUNT) for these particular cards, forcing the PIL to issue a CMD12 (STOP_TRANSMISSION)
void TestFragmentedWrite(TInt aLength)
	{
	test.Next( _L("Test a large write just bigger than PSL buffer size") );

	HBufC8* bigBuf = HBufC8::New( aLength);
	test( bigBuf != NULL );
	TPtr8 bigPtr( bigBuf->Des() );

	TInt64 startPos = 0;

	// for a dual-slot enabled H4, buffer size is 132K - (512 * 2) = 131K

	
	test.Printf( _L("Initializing buffer contents...\n"));
	bigPtr.SetLength(aLength);
	TInt n;
	for (n=0; n<aLength; n++)
		{
		bigPtr[n] = (TUint8) n;
		}

	bigPtr.SetLength(aLength);
	test.Printf( _L("Write %d sectors\n"), bigPtr.Length() / 512);
	TEST_FOR_ERROR( TheMmcDrive.Write( startPos, bigPtr) );


	bigPtr.SetLength(aLength);
	bigPtr.FillZ();

	test.Printf( _L("Read %d sectors\n"), bigPtr.Length() / 512);
	TEST_FOR_ERROR( TheMmcDrive.Read( startPos, bigPtr.Length(), bigPtr) );

	test.Printf( _L("Read #1 len %d \n"), bigPtr.Length());

	for (n=0; n< 0 + aLength; n++)
		{
		if (bigPtr[n] != (TUint8) n)
			{
			test.Printf(_L("mismatch at %lx [0x%02x] != [0x%02x]"), n, bigPtr[n], (TUint8) n);
			test(0);
			}
		}

	delete bigBuf;
	}

void TestWrite()
	{
	// for a dual-slot enabled H4, buffer size is 132K - (512 * 2) = 131K
	TestFragmentedWrite(131*1024 + 512);
	// for a single-slot enabled H4, buffer size is 132K - (512 * 1) = 131K + 512
	TestFragmentedWrite(131*1024 + 1024);


	test.Next( _L("Test writing to drive") );
	DoReadWriteTest( 0, 65536, EFalse );
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0166
@SYMTestCaseDesc Test MMC Card accesses at the end of the media
@SYMTestPriority High

@SYMTestActions  
	a. If the card is not read-only, perform read/write tests at the last 64K of the media.
	b. Test that all accesses beyond the end of the media produce an error.

@SYMTestExpectedResults All tests must pass

@SYMPREQ1389 CR0795 Support for >2G SD Cards
*/
void TestCapacity()
	{
	if(!IsReadOnly)
		{
		test.Next( _L("Test access at end of media") );
		DoReadWriteTest( TheMmcDrive.Size() - 65536, 65536, ETrue );
		}

	test.Printf( _L("Test accesses past end of media produce an error\n") );

	TBuf8<1024> buf;
	
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size(), 1, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size(), 2, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size(), 512, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() + 1, 512, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() + 512, 512, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() - 1, 2, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() - 511, 512, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() - 512, 513, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() - 65536, 65537, buf ) );
	test( KErrNone != TheMmcDrive.Read( TheMmcDrive.Size() - 512, 1024, buf ) );
	}


void WriteAcrossBoundaries(TInt64 aBoundary)
	{
	test.Printf( _L("Test for aliasing around boundary\n") );
	TBuf8<512> bufLo;
	TBuf8<512> bufHi;
	TBuf8<8192> bufRead;
	
	bufLo.Fill( 0xE4, 512 );
	bufHi.Fill( 0x19, 512 );

	TEST_FOR_ERROR( TheMmcDrive.Write( 0, bufLo ) );
	TEST_FOR_ERROR( TheMmcDrive.Write( aBoundary, bufHi ) );
	TEST_FOR_ERROR( TheMmcDrive.Read( 0, 512, bufRead ) );
	test( bufRead == bufLo );
	TEST_FOR_ERROR( TheMmcDrive.Read( aBoundary, 512, bufRead ) );
	test( bufRead == bufHi );

	bufHi.Fill( 0xBB, 1 );
	TEST_FOR_ERROR( TheMmcDrive.Write( aBoundary, bufHi ) );
	TEST_FOR_ERROR( TheMmcDrive.Read( 0, 512, bufRead ) );
	test( bufRead == bufLo );

	bufHi.Fill( 0xCC, 1 );
	TEST_FOR_ERROR( TheMmcDrive.Write( (aBoundary+1), bufHi ) );
	TEST_FOR_ERROR( TheMmcDrive.Read( 0, 512, bufRead ) );
	test( bufRead == bufLo );

	test.Printf( _L("Test write which ends at boundary\n") );
	bufHi.Fill( 0x33, 512 );
	TEST_FOR_ERROR( TheMmcDrive.Write( aBoundary, bufHi ) );
	TEST_FOR_ERROR( TheMmcDrive.Read( aBoundary, 512, bufRead ) );
	test( bufRead == bufHi );

	bufHi.Fill( 0x44, 512 );
	TEST_FOR_ERROR( TheMmcDrive.Write( aBoundary - 512, bufHi ) );
	TEST_FOR_ERROR( TheMmcDrive.Read( aBoundary - 512, 512, bufRead ) );
	test( bufRead == bufHi );

	TEST_FOR_ERROR( TheMmcDrive.Read( 0, 512, bufRead ) );
	test( bufRead == bufLo );

	bufHi.Fill( 0x33, 512 );
	TEST_FOR_ERROR( TheMmcDrive.Read( aBoundary, 512, bufRead ) );
	test( bufRead == bufHi );

	test.Printf( _L("Test read-modify-write across boundary\n") );
	TBuf8<512> rmw;
	TBuf8<8192> data;
	rmw.Fill( 0x66, 512 );
	data.Fill( 0x11, 8192 );
	
	for( TInt i = 1; i < 511; ++i )
		{
		ProgressBar(i, 511, 11);
	
		// Create initial data block
		TEST_FOR_ERROR( TheMmcDrive.Write( aBoundary - 512, data ) );

		// Read-modify-write some data
		TEST_FOR_ERROR( TheMmcDrive.Write( aBoundary - 512 + i, rmw ) );

		// Modify buffer to what we expect
		data.MidTPtr( i, 512 ) = rmw;

		// Read it back and check it matches
		TEST_FOR_ERROR( TheMmcDrive.Read( aBoundary - 512, 8192, bufRead ) );
		test( CompareBuffers( bufRead, data ) );
		}
	test.Printf(_L("\n"));
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0167
@SYMTestCaseDesc Test that the boundary >2GB doesn't produce aliases or errors
@SYMTestPriority High

@SYMTestActions  
	a. Test that writing at the 2G boundary does not produce aliases.
	b. Test writes that end at the 2G boundary.
	c. Test read/modify/write across the 2G boundary.

@SYMTestExpectedResults All tests must pass

@SYMPREQ1389 CR0795 Support for >2G SD Cards
*/
void TestBoundaries()
	{

	if( TheMmcDrive.Size() < 0x80008000 )
		{
		test.Printf( _L("Drive not large enough for 2GB boundary test... skipped\n") );
		return;
		}
		
	// Test that the boundary 2GB doesn't produce aliases or errors
	// >2Gb cards change addressing scheme from byte to block base
	test.Next( _L("Test 2GB boundary") );	
	WriteAcrossBoundaries(0x80000000);
	
// N.B. Commented Out for now due to compiler warnings	
//	if( TheMmcDrive.Size() < 0x100008000ll )
//			{
//			test.Printf( _L("Drive not large enough for 4GB boundary test... skipped\n") );
//			return;
//			}
//	// Test that the boundary 4GB doesn't produce aliases or errors
//	// >4GB cards change addressing scheme from 32bit to 64bit addresses
//	test.Next( _L("Test 4GB boundary") );	
//	WriteAcrossBoundaries(0x100000000ll); 
	}


/**
@SYMTestCaseID PBASE-T_MMCDRV-0512
@SYMTestCaseDesc Test Media Change/Capabilities Reporting
@SYMTestPriority High

@SYMTestActions
	    a.) Test Media Change flag after Media Change
		b.) Test Capabilities reporting for Out Of Memory Conditions
        c.) Test Media Change flag after Machine power-off
		d.) Test Capabilities reporting after Machine power-off
		e.) Test Multiple Media Change flags after Media Change

@SYMTestExpectedResults All tests must pass	
*/
void TestMediaChange()
	{
	test.Next(_L("MMC drive: Media change"));
#if defined (__WINS__)
	test.Printf( _L("<<<Hit F5 - then any other key>>>\r\n"));
#else
	test.Printf( _L("<<<Generate Media change - then hit a key>>>\r\n"));
#endif
	test.Getch();
	User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
	test(ChangeFlag!=EFalse);

	test.Next(_L("MMC drive: Caps following media change"));
	
	TLocalDriveCapsV4 info;
	TPckg<TLocalDriveCapsV4> infoPckg(info);
	
	test(TheMmcDrive.Caps(infoPckg)==KErrNone);
	test(info.iType==EMediaHardDisk);

	test.Next(_L("MMC drive: Caps while OOM"));
	TInt err;
	test.Printf(_L("Mount returns:"));
	for (TInt j=1;j<16;j++)
		{
		__KHEAP_SETFAIL(RHeap::EDeterministic,j);
		err=TheMmcDrive.Caps(infoPckg);
		test.Printf(_L("(%d)"),err);
		__KHEAP_RESET;
		}
	test.Printf(_L("\r\n"));

	test.Next(_L("MMC drive: Machine power-off."));
	ChangeFlag=EFalse;
	RTimer timer;
	TRequestStatus trs;
	test(timer.CreateLocal()==KErrNone);
	TTime tim;
	tim.HomeTime();
	tim+=TTimeIntervalSeconds(8);
	timer.At(trs,tim);
	UserHal::SwitchOff();
	User::WaitForRequest(trs);
	test(trs.Int()==KErrNone);
	test(ChangeFlag==EFalse);		// ie machine power off hasn't updated it

	test.Next(_L("MMC drive: Caps following power off"));
	TInt r=TheMmcDrive.Caps(infoPckg);
	test(r==KErrNone);
	test(info.iType==EMediaHardDisk);

	test.Next(_L("Starting 2nd thread"));
	SecThreadChangeFlag=EFalse;
	RThread thread;
	TRequestStatus stat;
	test(thread.Create(_L("Thread"),dontDisconnectThread,KDefaultStackSize,KHeapSize,KHeapSize,NULL)==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	test(stat==KErrNone);
	thread.Close();

	test.Next(_L("MMC drive: 2nd media change"));
//	UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change	
	test(ChangeFlag!=EFalse);
	test(SecThreadChangeFlag==EFalse); // Closed 2nd thread so shouldn't have been updated
	}
	

//// End of Test 
void Format()
//
// Format current drive
//
	{
	RFs TheFs;
	test(TheFs.Connect() == KErrNone);
	
	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(RFsDNum+'A');
	
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EQuickFormat,count);
	test(r==KErrNone);
	while(count)
		{
		TInt r=format.Next(count);
		test(r==KErrNone);
		}
	format.Close();
	}

void AllocateBuffers()
	{
	test.Next(_L("Allocate Buffers"));

	//HBufC8* wrBufH = NULL;
	//HBufC8* rdBufH = NULL;

	wrBufH = HBufC8::New(KVeryLongRdWrBufLen);
	test(wrBufH != NULL);

	rdBufH = HBufC8::New(KVeryLongRdWrBufLen);
	if(rdBufH == NULL) delete wrBufH;
	test(rdBufH != NULL);

	wrBuf.Set(wrBufH->Des());
	rdBuf.Set(rdBufH->Des());
	}
	
void AllocateSharedBuffers(TBool Fragmented, TBool Caching)
	{
	// Setup SharedMemory Buffers
	test.Next(_L("Allocate Shared Memory\n"));
	
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	test.Printf(_L("Initialise\n"));
	TInt r = UserHal::PageSizeInBytes(PageSize);
	test(r==KErrNone);

	test.Printf(_L("Loading test driver\n"));
	r = User::LoadLogicalDevice(KSharedChunkLddName);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Printf(_L("Opening channel\n"));
	r = Ldd.Open();
	test(r==KErrNone);

	test.Printf(_L("Create chunk\n"));
	
	TUint aCreateFlags = EMultiple|EOwnsMemory;
	
	if (Caching)
		{
		test.Printf(_L("Chunk Type:Caching\n"));
		aCreateFlags |= ECached;
		}
	else
		test.Printf(_L("Chunk Type:Fully Blocking\n"));
	
    TCommitType aCommitType = EContiguous;
      
    TUint TotalChunkSize = ChunkSize;  // rounded to nearest Page Size
    
	TUint ChunkAttribs = TotalChunkSize|aCreateFlags;	
	r = Ldd.CreateChunk(ChunkAttribs);
	test(r==KErrNone);

	if(Fragmented)
		{
		test.Printf(_L("Commit Fragmented Memory\n"));
			
		// Allocate Pages in reverse order to maximise memory fragmentation
		TUint i = ChunkSize;
		do
			{
			i-=PageSize;
			test.Printf(_L("Commit %d\n"), i);
			r = Ldd.CommitMemory(aCommitType|i,PageSize);
			test(r==KErrNone);
			}while (i>0);
		}
	else
		{
		test.Printf(_L("Commit Contigouos Memory\n"));
		r = Ldd.CommitMemory(aCommitType,TotalChunkSize);
		test(r==KErrNone);
		}

	test.Printf(_L("Open user handle\n"));
	r = Ldd.GetChunkHandle(TheChunk);
	test(r==KErrNone);
	
	}


void DeAllocateBuffers()
	{
	delete rdBufH;
	delete wrBufH;
	}

void DeAllocareSharedMemory()
	{
// destory chunk
	test.Printf(_L("Shared Memory\n"));
	test.Printf(_L("Close user chunk handle\n"));
	TheChunk.Close();

	test.Printf(_L("Close kernel chunk handle\n"));
	TInt r = Ldd.CloseChunk();  // 1==DObject::EObjectDeleted
	test(r==1);

	test.Printf(_L("Check chunk is destroyed\n"));
	r = Ldd.IsDestroyed();
	test(r==1);
        
	test.Printf(_L("Close test driver\n"));
	Ldd.Close();
	}


TBool SetupDrivesForPlatform(TInt& aDrive, TInt &aRFsDriveNum)
/**
 * Finds a MMC/SD suitable drive for testing
 *
 * @param aDrive  The number of the local drive to test
 * @return TBool ETrue if a suitable drive is found, EFalse otherwise.
 */
	{
	
	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();

	test.Printf(_L(" iRegisteredDriveBitmask 0x%08X"), di.iRegisteredDriveBitmask);

	aDrive  = -1;
	
	TLocalDriveCapsV5Buf capsBuf;
	TBusLocalDrive TBLD;
	TLocalDriveCapsV5& caps = capsBuf();
	TPtrC8 localSerialNum;
	TInt registeredDriveNum = 0;
	for(aDrive=0; aDrive < KMaxLocalDrives; aDrive++)
		{
		TInt driveNumberMask = 1 << aDrive;
		if ((di.iRegisteredDriveBitmask & driveNumberMask) == 0)
			continue;

		test.Printf(_L(" Drive %d -  %S\r\n"), aDrive, &di.iDriveName[registeredDriveNum]);

		// check that the card is readable (so we can ignore for empty card slots)
		if ((di.iDriveName[registeredDriveNum].MatchF(_L("MultiMediaCard0")) == KErrNone) ||
		    (di.iDriveName[registeredDriveNum].MatchF(_L("SDIOCard0")) == KErrNone))
			{
			
			TBool TBLDChangedFlag;
			TInt r = TBLD.Connect(aDrive, TBLDChangedFlag);
//test.Printf(_L(" Connect returned %d\n"), r);
			if (r == KErrNone)
				{
				r = TBLD.Caps(capsBuf);
				localSerialNum.Set(caps.iSerialNum, caps.iSerialNumLength);
				const TInt KSectSize = 512;
				TBuf8<KSectSize> sect;
				r = TBLD.Read(0, KSectSize, sect);
//test.Printf(_L(" Read returned %d\n"), r);
				
				TBLD.Disconnect();
				if (r == KErrNone)
					break;
				}
			}
		registeredDriveNum++;
		}

	if(aDrive == KMaxLocalDrives)
		{
		test.Printf(_L(" MMC Drive Not Found\r\n"));
		return EFalse;
		}

	// Work out the file server drive number (which isn't necessarily the same 
	// as the TBusLocalDrive drive number)
	RFs theFs;
	test(theFs.Connect() == KErrNone);

	TInt i;
	for (i = EDriveA; i < EDriveZ; i++)
		{
		TMediaSerialNumber serialNum;
	    TInt r = theFs.GetMediaSerialNumber(serialNum, i);
		TInt len = serialNum.Length();
		TInt n;
		for (n=0; n<len; n+=16)
		{
		TBuf16<16*3 +1> buf;
			for (TInt m=n; m<n+16; m++)
				{
				TBuf16<3> hexBuf;
				hexBuf.Format(_L("%02X "),serialNum[m]);
				buf.Append(hexBuf);
				}
		buf.Append(_L("\n"));
		test.Printf(buf);
		}
		if (serialNum.Compare(localSerialNum) == 0)
			{
			TVolumeInfo vi;
	        r = theFs.Volume(vi, i);
			TBool sizeMatch = (vi.iSize < caps.iSize);
			if (sizeMatch)
				{
				aRFsDriveNum = i;
				break;
				}
			}
		
		}
	if (i == EDriveZ)
		{
		test.Printf(_L(" RFs MMC Drive Not Found\r\n"));
		return EFalse;
		}

	theFs.Close();

	return ETrue;
	}


LOCAL_D TBool ParseCommandLineArgs()
	{
	
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);

    for (TPtrC token=lex.NextToken(); token.Length() != 0;token.Set(lex.NextToken()))
		{
		if (token.CompareF(_L("-m"))== 0)
			{
			ManualMode = ETrue;
			continue;
			}
		}
	
	if (ManualMode)
		{
		// Get the list of drives
		TDriveInfoV1Buf diBuf;
		UserHal::DriveInfo(diBuf);
		TDriveInfoV1 &di=diBuf();
		TInt driveCount = di.iTotalSupportedDrives;
		
		//Print the list of usable drives
		test.Printf(_L("\nDRIVES USED AT PRESENT :\r\n"));

		for (TInt i=0; i < driveCount; i++)
			{
			TBool flag=EFalse;
			RLocalDrive d;
			TInt r=d.Connect(i,flag);
			//Not all the drives are used at present
			if (r == KErrNotSupported)
				continue;

			test.Printf(_L("%d : DRIVE NAME  :%- 16S\r\n"), i, &di.iDriveName[i]);
			}	
		
		test.Printf(_L("\r\nWarning - all data on removable drive will be lost.\r\n"));
		test.Printf(_L("<<<Hit mmc drive number to continue>>>\r\n"));

		TChar driveToTest;
		driveToTest=(TUint)test.Getch();
		DriveNumber=((TUint)driveToTest) - '0';
		test(DriveNumber >= 1 && DriveNumber < di.iTotalSupportedDrives);
		
		return ETrue;
		}
	else
		{
		//Auto Mode
		//Lets find an MMC Drive to Test with....		
		return SetupDrivesForPlatform(DriveNumber, RFsDNum);
		}
	}


GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Test the MultiMediaCard (MMC) media driver"));

	if (!ParseCommandLineArgs())
		{
		test.Printf(_L("MMC Drive Not Found - Skipping test\r\n"));
		test.End();
		return(0);
		}
	
	AllocateBuffers();

	test.Printf(_L("Connect to local drive (%d)\n"),DriveNumber);

	ChangeFlag=EFalse;
	test(TheMmcDrive.Connect(DriveNumber,ChangeFlag)==KErrNone);

	TTime startTime;
	startTime.HomeTime();
	
	IsReadOnly = TestDriveInfo();

	// The following line causes t_mmcdrv to jump to the tests that check if the
	// mmc driver will carry on reading when the door is opened, but abort with
	// KErrGeneral when it is not.	Enabling the goto here is useful because it
	// allows the tester to skip the long read and write tests, which can take several
	// minutes on a 16Mb card, and longer if tracing is enabled.  It also stops the test
	// from returning when !mediaChangeSupported and not getting to the door opening tests.

#if TEST_DOOR_CLOSE
	goto doorTest;
#endif
	
	for(TInt pass = 0; pass < TMMCDrive::EMaxTestModes; pass++) 
		{
		TInt r = KErrNone;
		switch (pass)
			{			
			case 0 : r = TheMmcDrive.SetTestMode(TMMCDrive::ETestPartition); break;
			case 1 : 
				// don't trash partition table in automated mode because...
				// cards in test rigs have often got deliberately small partition sizes to testing (!)
				if (!ManualMode)
					continue;
				r = TheMmcDrive.SetTestMode(TMMCDrive::ETestWholeMedia); 
				break; 
			case 2 : {
						r = TheMmcDrive.SetTestMode(TMMCDrive::ETestSharedMemory);
						AllocateSharedBuffers(EFalse,EFalse);
						break;
					 }
			case 3 : {
						r = TheMmcDrive.SetTestMode(TMMCDrive::ETestSharedMemoryCache); 
						AllocateSharedBuffers(EFalse, ETrue);
						break;
					 }
			case 4 : {
						r = TheMmcDrive.SetTestMode(TMMCDrive::ETestSharedMemoryFrag);
						AllocateSharedBuffers(ETrue, EFalse);
						break;
			         }
			default: {
						r = TheMmcDrive.SetTestMode(TMMCDrive::ETestSharedMemoryFragCache);
						AllocateSharedBuffers(ETrue, ETrue);
						break;
			         }
			}


		if(r == KErrNone)
			{
			TestRead();
			TestCapacity();
 
			if(IsReadOnly == EFalse)
				{
				TestMultipleBlockReads();
				TestSectorReadWrite();
				TestWrite();
				TestBoundaries();
				TestFormat();
				}
			}
		
		if (pass > 1)
			{
			// Shared memory Test Mode in use
			DeAllocareSharedMemory();
			}
		}

	if (mediaChangeSupported)
		{
		// Remainder of tests involve media change
		TestMediaChange();
		
		#if TEST_DOOR_CLOSE
doorTest:
		#endif
		test.Next(_L("Launching 1.0Mb Read to interrupt with media change.\n"));
		TestHugeReadWrite(ETrue, 512 * 1024);

		test.Next(_L("Launching 1.0Mb Write to interrupt with media change.\n"));
		TestHugeReadWrite(EFalse, 512 * 1024);
		}
		
	TTime endTime;
	endTime.HomeTime();
	TTimeIntervalMicroSeconds elapsed=endTime.MicroSecondsFrom(startTime);
	test.Printf(_L("\n\r   (Elapsed time: %dmS)\r\n"),(elapsed.Int64()/1000));
	
	test.Printf(_L("Disconnect from local drive (%d)"),DriveNumber);
	TheMmcDrive.Disconnect();

	DeAllocateBuffers();

	// Format card with a File System i.e. FAT
	// Such that it is re-usable by next test
	Format();
	
	test.End();

	return(0);
	}
  
