// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Tests for the blockmap API. The blockmap API enumerates the resources
// used by a file depending upon the type of media on which the file
// resides.
// 002 Test Initialise error checking
// 003 Test processing of user-side block map into extent list
// 004 Test processing of user-side block map into extent list, block size > read unit size
// 005 Test Read error checking
// 006 Test Read
// Test BlockMap API functionality.
// 
//

//! @SYMTestCaseID			KBASE-T_BLOCKMAP-0337
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Blockmap tests
//! @SYMTestActions			001 Unit tests the TKernBlockMap class
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented


#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <f32file.h>
#include <e32math.h>
#include <hal.h>
#include "t_server.h"

RTest test( _L("T_BLOCKMAP") );

const TInt KReadBufferSize = 1024;
const TInt KMaxFragmentSize = 400;
const TInt KMaxFileSize = 10000;

TInt RamFatDrive = -1;
TInt RemovableFatDrive = -1;
TInt InternalRemovableFatDrive = -1;
TInt NandDrive = -1;
TBool Pageable = EFalse;
TBool Finished = EFalse;

enum DriveType
	{
	EDriveNand,
	EDriveRam,
	EDriveRemovable,
	EDriveInternalRemovable
	};

_LIT( KTestFile, "Z:\\TEST\\T_FILE.CPP");
_LIT( KTestFileFAT, "Z:\\Multiple\\T_file.cpp");
_LIT( KTestFileName, "T_FILE.CPP");
_LIT( KFragmentedFileName1, "FRAG1.TXT");
_LIT( KFragmentedFileName2, "FRAG2.TXT");
_LIT( KDriveBase, " :\\" );

LOCAL_C TInt TestBlockMapNandFATUserData(TInt64 aStartPos, TInt64 aEndPos)
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TFileName name(KDriveBase);
	name[0] = TText('A' + NandDrive);
	name.Append( KTestFileName );

	TInt r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	r = fMan->Delete(name);

	r = fMan->Copy(KTestFile, name);
	test_KErrNone(r);

	TInt localDriveNum = 0;
	RFile testFile;
	r = testFile.Open( TheFs, name, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt counter = 0;
	TInt startPos = aStartPos;
	TInt bmErr;

	// Store SBlockMapInfo objects in map:RArray until KErrCompletion is returned.
	do
		{
		bmErr = testFile.BlockMap(info, aStartPos, aEndPos, ETestDebug);
		if (bmErr != 0 && bmErr != KErrCompletion)
			{
			map.Close();
			testFile.Close();
			r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
			test_KErrNone(r);
			r = fMan->Delete(name);
			test_KErrNone(r);
			delete fMan;
			return bmErr;
			}
		map.Append(info); 
		if (counter++ == 0)
			localDriveNum = info.iLocalDriveNumber;
		} while ( bmErr == 0 && bmErr != KErrCompletion );
	test( bmErr == KErrCompletion );
	TInt granularity;

	TInt size;
	r = testFile.Size(size);
	test_KErrNone(r);

	TBuf8<KReadBufferSize> buf1;
	TBuf8<KReadBufferSize> buf2;
	
	TBool changed;	
	TBusLocalDrive localDrive;

	UserSvr::UnlockRamDrive();

	TBlockMapEntry* myBlockMapEntry;
	TInt myCounter = 0;
	TInt totalSegments = 0;
	TInt remainder = 0;
	TInt miniLength = 0;
	TInt amountRead = 0;

	TInt c;
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		totalSegments += granularity;
		}

	const TInt KTotalSegments = totalSegments;
	r = localDrive.Connect( localDriveNum, changed );
	test_KErrNone(r);

//	For each SBlockMapInfo object in RArray map
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		TInt length;
		if ( aEndPos == -1 )
			{
			length = size - startPos;
			aEndPos = size;
			}
		else
			length = aEndPos - startPos;
		
		for ( TInt c2 = 1; c2 <= granularity; c2++)
			{
			myCounter = 0;
			if ( c2 == KTotalSegments && aEndPos%map[c].iBlockGranularity != 0 )
				remainder = map[c].iBlockGranularity - aEndPos%map[c].iBlockGranularity;
			else
				remainder = 0;
			miniLength = map[c].iBlockGranularity*myBlockMapEntry->iNumberOfBlocks - remainder - (c2 == 1?map[c].iBlockStartOffset:0);
			do
				{
				if ( miniLength >= KReadBufferSize )
					{
					testFile.Read( startPos + amountRead, buf1, KReadBufferSize );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, KReadBufferSize, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					buf1.Zero();
					buf2.Zero();
					myCounter++;
					miniLength -= KReadBufferSize;
					length -= KReadBufferSize;
					amountRead += KReadBufferSize;
					}
				else
					{
					testFile.Read(startPos + amountRead, buf1, miniLength);
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, miniLength, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					amountRead += miniLength;
					length -= miniLength;
					miniLength = 0;
					}
				} while ( miniLength != 0 && length != 0);
			myBlockMapEntry++;
			}
		}
	map.Close();

	testFile.Close();
	r=fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	test_KErrNone(r);
	r = fMan->Delete(name);
	test_KErrNone(r);
	delete fMan;
	return bmErr;
	}

LOCAL_C TInt TestBlockMapNandFAT(TInt64 aStartPos, TInt64 aEndPos)
//
// Test BlockMap retrieval on NAND FAT.
//
	{
	TInt localDriveNum = 0;
	RFile testFile;
	TInt r = testFile.Open( TheFs, KTestFileFAT, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt counter = 0;
	TInt startPos = aStartPos;
	TInt bmErr;

	// Store SBlockMapInfo objects in map:RArray until KErrCompletion is returned.
	do
		{
		bmErr = testFile.BlockMap(info, aStartPos, aEndPos, Pageable?EBlockMapUsagePaging:ETestDebug);
		if (bmErr != 0 && bmErr != KErrCompletion)
			{
			map.Close();
			testFile.Close();
			return bmErr;
			}
		map.Append(info); 
		if (counter++ == 0)
			localDriveNum = info.iLocalDriveNumber;
		} while ( bmErr == 0 && bmErr != KErrCompletion );
	test( bmErr == KErrCompletion );
	TInt granularity;

	TInt size;
	r = testFile.Size(size);
	test_KErrNone(r);

	TBuf8<KReadBufferSize> buf1;
	TBuf8<KReadBufferSize> buf2;
	
	TBool changed;	
	TBusLocalDrive localDrive;

	TBlockMapEntry* myBlockMapEntry;
	TInt myCounter = 0;
	TInt totalSegments = 0;
	TInt remainder = 0;
	TInt miniLength = 0;
	TInt amountRead = 0;

	TInt c;
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		totalSegments += granularity;
		}

	const TInt KTotalSegments = totalSegments;
	r = localDrive.Connect( localDriveNum, changed );
	test_KErrNone(r);

//	For each SBlockMapInfo object in RArray map
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);

		TInt length;
		if ( aEndPos == -1 )
			{
			length = size - startPos;
			aEndPos = size;
			}
		else
			length = aEndPos - startPos;

		for ( TInt c2 = 1; c2 <= granularity; c2++)
			{
			myCounter = 0;
			if ( c2 == KTotalSegments && aEndPos%map[c].iBlockGranularity != 0 )
				remainder = map[c].iBlockGranularity - aEndPos%map[c].iBlockGranularity;
			else
				remainder = 0;
			miniLength = map[c].iBlockGranularity*myBlockMapEntry->iNumberOfBlocks - remainder - (c2 == 1?map[c].iBlockStartOffset:0);
			do
				{
				if ( miniLength >= KReadBufferSize )
					{
					testFile.Read( startPos + amountRead, buf1, KReadBufferSize );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, KReadBufferSize, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					buf1.Zero();
					buf2.Zero();
					myCounter++;
					miniLength -= KReadBufferSize;
					length -= KReadBufferSize;
					amountRead += KReadBufferSize;
					}
				else
					{
					testFile.Read(startPos + amountRead, buf1, miniLength);
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, miniLength, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					amountRead += miniLength;
					length -= miniLength;
					miniLength = 0;
					}
				} while ( miniLength != 0 && length != 0);
			myBlockMapEntry++;
			}
		}
	map.Close();
	testFile.Close();
	return bmErr;
	}

LOCAL_C TInt TestBlockMapNandROFS(TInt64 aStartPos, TInt64 aEndPos)
//
// Test BlockMap retrieval on NAND ROFS.
//
	{
	TInt localDriveNum = 0;
	RFile testFile;
	TInt r = testFile.Open( TheFs, KTestFile, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt counter = 0;
	TInt startPos = aStartPos;
	TInt bmErr;

	// Store SBlockMapInfo objects in map:RArray until KErrCompletion is returned.
	do
		{
		bmErr = testFile.BlockMap(info, aStartPos, aEndPos, Pageable?EBlockMapUsagePaging:ETestDebug);
		if (bmErr != 0 && bmErr != KErrCompletion)
			{
			map.Close();
			testFile.Close();
			return bmErr;
			}
		map.Append(info); 
		if (counter++ == 0)
			localDriveNum = info.iLocalDriveNumber;
		} while ( bmErr == 0 && bmErr != KErrCompletion );
	test( bmErr == KErrCompletion );
	TInt granularity;

	TInt size;
	r = testFile.Size(size);
	test_KErrNone(r);

	TBuf8<KReadBufferSize> buf1;
	TBuf8<KReadBufferSize> buf2;
	
	TBool changed;	
	TBusLocalDrive localDrive;

	TBlockMapEntry* myBlockMapEntry;
	TInt myCounter = 0;
	TInt totalSegments = 0;
	TInt miniLength = 0;
	TInt amountRead = 0;

	TInt c;
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		totalSegments += granularity;
		}
	r = localDrive.Connect( localDriveNum, changed );
	test_KErrNone(r);

//	For each SBlockMapInfo object in RArray map
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);

		TInt length;
		if ( aEndPos == -1 )
			{
			length = size - startPos;
			aEndPos = size;
			}
		else
			length = aEndPos - startPos;
		
		for ( TInt c2 = 1; c2 <= granularity; c2++)
			{
			myCounter = 0;
			miniLength = length;
			do
				{
				if ( miniLength >= KReadBufferSize )
					{
					testFile.Read( startPos + amountRead, buf1, KReadBufferSize );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, KReadBufferSize, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					buf1.Zero();
					buf2.Zero();
					myCounter++;
					miniLength -= KReadBufferSize;
					length -= KReadBufferSize;
					amountRead += KReadBufferSize;
					}
				else
					{
					testFile.Read(startPos + amountRead, buf1, miniLength);
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, miniLength, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					amountRead += miniLength;
					length -= miniLength;
					miniLength = 0;
					}
				} while ( miniLength != 0 && length != 0);
			myBlockMapEntry++;
			}
		}
	map.Close();

	testFile.Close();
	return bmErr;
	}

LOCAL_C TInt TestBlockMapRamFAT(TInt64 aStartPos, TInt64 aEndPos)
//
// Test BlockMap retrieval on RAM FAT.
//
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TFileName name(KDriveBase);
	name[0] = TText('A' + RamFatDrive);
	name.Append( KTestFileName );

	TInt r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	r = fMan->Delete(name);

	r = fMan->Copy(KTestFile, name);
	test_Value(r, r == KErrNone  || r == KErrAlreadyExists);

	TInt localDriveNum = 0;
	RFile testFile;
	r = testFile.Open( TheFs, name, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt counter = 0;
	TInt startPos = aStartPos;
	TInt bmErr;

	// Store SBlockMapInfo objects in map:RArray until KErrCompletion is returned.
	do
		{
		bmErr = testFile.BlockMap(info, aStartPos, aEndPos, ETestDebug);
		if (bmErr != 0 && bmErr != KErrCompletion)
			{
			map.Close();
			testFile.Close();
			r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
			test_KErrNone(r);
			r = fMan->Delete(name);
			test_KErrNone(r);
			delete fMan;
			return bmErr;
			}
		map.Append(info); 
		if (counter++ == 0)
			localDriveNum = info.iLocalDriveNumber;
		} while ( bmErr == 0 && bmErr != KErrCompletion );
	test( bmErr == KErrCompletion );
	TInt granularity;

	TInt size;
	r = testFile.Size(size);
	test_KErrNone(r);

	TBuf8<KReadBufferSize> buf1;
	TBuf8<KReadBufferSize> buf2;
	
	TBool changed;	
	TBusLocalDrive localDrive;

	UserSvr::UnlockRamDrive();

	TBlockMapEntry* myBlockMapEntry;
	TInt myCounter = 0;
	TInt totalSegments = 0;
	TInt remainder = 0;
	TInt miniLength = 0;
	TInt amountRead = 0;

	TInt c;
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		totalSegments += granularity;
		}

	const TInt KTotalSegments = totalSegments;

	r = localDrive.Connect( localDriveNum, changed );
	test_KErrNone(r);

//	For each SBlockMapInfo object in RArray map
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);

		TInt length;
		if ( aEndPos == -1 )
			{
			length = size - startPos;
			aEndPos = size;
			}
		else
			length = aEndPos - startPos;
	
		for ( TInt c2 = 1; c2 <= granularity; c2++)
			{
			myCounter = 0;
			if ( c2 == KTotalSegments && aEndPos%map[c].iBlockGranularity != 0 )
				remainder = map[c].iBlockGranularity - aEndPos%map[c].iBlockGranularity;
			else
				remainder = 0;
			miniLength = map[c].iBlockGranularity*myBlockMapEntry->iNumberOfBlocks - remainder - (c2 == 1?map[c].iBlockStartOffset:0);
			do
				{
				if ( miniLength >= KReadBufferSize )
					{
					testFile.Read( startPos + amountRead, buf1, KReadBufferSize );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, KReadBufferSize, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					buf1.Zero();
					buf2.Zero();
					myCounter++;
					miniLength -= KReadBufferSize;
					length -= KReadBufferSize;
					amountRead += KReadBufferSize;
					}
				else
					{
					testFile.Read(startPos + amountRead, buf1, miniLength);
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, miniLength, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					amountRead += miniLength;
					length -= miniLength;
					miniLength = 0;
					}
				} while ( miniLength != 0 && length != 0);
			myBlockMapEntry++;
			}
		}
	map.Close();

	testFile.Close();
	r=fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	test_KErrNone(r);
	r = fMan->Delete(name);
	test_KErrNone(r);
	delete fMan;
	return bmErr;
	}

LOCAL_C TInt TestBlockMapRamFAT2(TInt64 aStartPos, TInt64 aEndPos)
//
// Test BlockMap retrieval on Ram FAT.
//
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TFileName name(KDriveBase);
	name[0] = TText('A' + RamFatDrive);
	name.Append( KTestFileName );

	TInt r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	r = fMan->Delete(name);

	r = fMan->Copy(KTestFile, name);
	test_Value(r, r == KErrNone  || r == KErrAlreadyExists);

	RFile testFile;
	r = testFile.Open( TheFs, name, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;

	TInt bmErr;
	bmErr = testFile.BlockMap(info, aStartPos, aEndPos, EBlockMapUsagePaging);

	map.Close();

	testFile.Close();
	r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	test_KErrNone(r);
	r = fMan->Delete(name);
	test_KErrNone(r);
	delete fMan;
	return bmErr;
	}

LOCAL_C TInt TestBlockMapRemovableFAT(TInt64 aStartPos, TInt64 aEndPos)
//
// Test BlockMap retrieval on Removable FAT.
//
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TFileName name(KDriveBase);
	name[0] = TText('A' + RemovableFatDrive);
	name.Append( KTestFileName );

	TInt r=fMan->Copy(KTestFile, name);
	test_Value(r, r == KErrNone  || r == KErrAlreadyExists);

	RFile testFile;
	r = testFile.Open( TheFs, name, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt bmErr = testFile.BlockMap(info, aStartPos, aEndPos, Pageable?EBlockMapUsagePaging:ETestDebug);
	map.Close();

	testFile.Close();
	r=fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	test_KErrNone(r);
	r = fMan->Delete(name);
	test_KErrNone(r);
	delete fMan;
	return bmErr;
	}

LOCAL_C TInt TestBlockMapInternalRemovableFAT(TInt64 aStartPos, TInt64 aEndPos)
//
// Test BlockMap retrieval on internal removable FAT.
//
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TFileName name(KDriveBase);
	name[0] = TText('A' + RamFatDrive);
	name.Append( KTestFileName );

	TInt r=fMan->Copy(KTestFile, name);
	test_Value(r, r == KErrNone  || r == KErrAlreadyExists);

	TInt localDriveNum = 0;
	RFile testFile;
	r = testFile.Open( TheFs, name, EFileRead );
	test_KErrNone(r);
	
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt counter = 0;
	TInt startPos = aStartPos;
	TInt bmErr;

	// Store SBlockMapInfo objects in map:RArray until KErrCompletion is returned.
	do
		{
		bmErr = testFile.BlockMap(info, aStartPos, aEndPos, ETestDebug);
		if (bmErr != 0 && bmErr != KErrCompletion)
			{
			map.Close();
			testFile.Close();
			r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
			test_KErrNone(r);
			r = fMan->Delete(name);
			test_KErrNone(r);
			delete fMan;
			return bmErr;
			}
		map.Append(info); 
		if (counter++ == 0)
			localDriveNum = info.iLocalDriveNumber;
		} while ( bmErr == 0 && bmErr != KErrCompletion );
	test( bmErr == KErrCompletion );
	TInt granularity;

	TInt size;
	r = testFile.Size(size);
	test_KErrNone(r);

	TBuf8<KReadBufferSize> buf1;
	TBuf8<KReadBufferSize> buf2;
	
	TBool changed;	
	TBusLocalDrive localDrive;

	UserSvr::UnlockRamDrive();

	TBlockMapEntry* myBlockMapEntry;
	TInt myCounter = 0;
	TInt totalSegments = 0;
	TInt remainder = 0;
	TInt miniLength = 0;
	TInt amountRead = 0;

	TInt c;
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		totalSegments += granularity;
		}

	const TInt KTotalSegments = totalSegments;

	r = localDrive.Connect( localDriveNum, changed );
	test_KErrNone(r);

//	For each SBlockMapInfo object in RArray map
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);

		TInt length;
		if ( aEndPos == -1 )
			{
			length = size - startPos;
			aEndPos = size;
			}
		else
			length = aEndPos - startPos;
		
		for ( TInt c2 = 1; c2 <= granularity; c2++)
			{
			myCounter = 0;
			if ( c2 == KTotalSegments && aEndPos%map[c].iBlockGranularity != 0 )
				remainder = map[c].iBlockGranularity - aEndPos%map[c].iBlockGranularity;
			else
				remainder = 0;
			miniLength = map[c].iBlockGranularity*myBlockMapEntry->iNumberOfBlocks - remainder - (c2 == 1?map[c].iBlockStartOffset:0);
			do
				{
				if ( miniLength >= KReadBufferSize )
					{
					testFile.Read( startPos + amountRead, buf1, KReadBufferSize );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, KReadBufferSize, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					buf1.Zero();
					buf2.Zero();
					myCounter++;
					miniLength -= KReadBufferSize;
					length -= KReadBufferSize;
					amountRead += KReadBufferSize;
					}
				else
					{
					testFile.Read(startPos + amountRead, buf1, miniLength);
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, miniLength, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					amountRead += miniLength;
					length -= miniLength;
					miniLength = 0;
					}
				} while ( miniLength != 0 && length != 0);
			myBlockMapEntry++;
			}
		}
	map.Close();

	testFile.Close();
	r=fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
	test_KErrNone(r);
	r = fMan->Delete(name);
	test_KErrNone(r);
	delete fMan;
	return bmErr;
	}

LOCAL_C TInt TestBlockMapFragmented(DriveType aDriveType, TInt64 aStartPos, TInt64 aEndPos)
	{
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	TFileName name(KDriveBase);
	if (aDriveType==EDriveRam)
		name[0] = TText('A' + RamFatDrive);
	else if (aDriveType==EDriveRemovable)
		name[0] = TText('A' + RemovableFatDrive);
	else if (aDriveType==EDriveNand)
		name[0] = TText('A' + NandDrive);
	else 
		name[0] = TText('A' + InternalRemovableFatDrive);
	name.Append( KFragmentedFileName1 );
	TInt localDriveNum = 0;
	RFile testFile;
	TInt r = testFile.Open( TheFs, name, EFileRead );
	test_KErrNone(r);
	RArray<SBlockMapInfo> map;	// From RArray<TBlockMapEntry> map; to RArray<SBlockMapInfo> map;
	SBlockMapInfo info;
	TInt counter = 0;
	TInt startPos = aStartPos;
	TInt bmErr;

	// Store SBlockMapInfo objects in map:RArray until KErrCompletion is returned.
	do
		{
		bmErr = testFile.BlockMap(info, aStartPos, aEndPos, ETestDebug);
		if (bmErr != 0 && bmErr != KErrCompletion)
			{
			map.Close();
			testFile.Close();
			if ( Finished )
				{
				r = fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
				test_KErrNone(r);
				r = fMan->Delete(name);
				test_KErrNone(r);
				}
			delete fMan;
			return bmErr;
			}
		map.Append(info); 
		if (counter++ == 0)
			localDriveNum = info.iLocalDriveNumber;
		} while ( bmErr == 0 && bmErr != KErrCompletion );
	test( bmErr == KErrCompletion );
	TInt granularity;
	TInt size;
	r = testFile.Size(size);
	test_KErrNone(r);

	TBuf8<KReadBufferSize> buf1;
	TBuf8<KReadBufferSize> buf2;
	
	TBool changed;	
	TBusLocalDrive localDrive;

	UserSvr::UnlockRamDrive();

	TBlockMapEntry* myBlockMapEntry;
	TInt myCounter = 0;
	TInt totalSegments = 0;
	TInt remainder = 0;
	TInt miniLength = 0;
	TInt amountRead = 0;

	TInt c;
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);
		totalSegments += granularity;
		}

	const TInt KTotalSegments = totalSegments;
	r = localDrive.Connect( localDriveNum, changed );
	test_KErrNone(r);

//	For each SBlockMapInfo object in RArray map
	for ( c = 0; c < map.Count(); c++ )
		{
		myBlockMapEntry = (TBlockMapEntry*) map[c].iMap.Ptr();
		granularity = map[c].iMap.Size()/sizeof(TBlockMapEntry);

		TInt length;
		if ( aEndPos == -1 )
			{
			length = size - startPos;
			aEndPos = size;
			}
		else
			length = aEndPos - startPos;
		
		for ( TInt c2 = 1; c2 <= granularity; c2++)
			{
			myCounter = 0;
			if ( c2 == KTotalSegments && aEndPos%map[c].iBlockGranularity != 0 )
				remainder = map[c].iBlockGranularity - aEndPos%map[c].iBlockGranularity;
			else
				remainder = 0;
			miniLength = map[c].iBlockGranularity*myBlockMapEntry->iNumberOfBlocks - remainder - (c2 == 1?map[c].iBlockStartOffset:0);
			do
				{
				if ( miniLength >= KReadBufferSize )
					{
					testFile.Read( startPos + amountRead, buf1, KReadBufferSize );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, KReadBufferSize, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					buf1.Zero();
					buf2.Zero();
					myCounter++;
					miniLength -= KReadBufferSize;
					length -= KReadBufferSize;
					amountRead += KReadBufferSize;
					}
				else
					{
					testFile.Read(startPos + amountRead, buf1, miniLength );
					localDrive.Read( map[c].iStartBlockAddress + myBlockMapEntry->iStartBlock * map[c].iBlockGranularity + (c2 == 1?map[c].iBlockStartOffset:0) + myCounter*KReadBufferSize, miniLength, buf2);
					r = buf1.Compare( buf2 );
					test_Value(r, r == 0 );
					amountRead += miniLength;
					length -= miniLength;
					miniLength = 0;
					}
				} while ( miniLength != 0 && length != 0);
			myBlockMapEntry++;
			}
		}
	map.Close();

	testFile.Close();
	if ( Finished )
		{
		r=fMan->Attribs(name, 0, KEntryAttReadOnly, 0);
		test_KErrNone(r);
		r = fMan->Delete(name);
		test_KErrNone(r);
		}
	delete fMan;
	return bmErr;	
	}

LOCAL_C void GenerateFragmentedFiles(DriveType aDriveType)
	{
	TInt r;
	TFileName name1(KDriveBase);
	if (aDriveType==EDriveRam)
		name1[0] = TText('A' + RamFatDrive);
	else if (aDriveType==EDriveRemovable)
		name1[0] = TText('A' + RemovableFatDrive);
	else if (aDriveType==EDriveNand)
		name1[0] = TText('A' + NandDrive);
	else 
		name1[0] = TText('A' + InternalRemovableFatDrive);
	name1.Append( KFragmentedFileName1 );
	RFile file1;
	r = file1.Create(TheFs, name1, EFileWrite);
	test_KErrNone(r);
	file1.Close();

	TFileName name2(KDriveBase);
	if (aDriveType==EDriveRam)
		name2[0] = TText('A' + RamFatDrive);
	else if (aDriveType==EDriveRemovable)
		name2[0] = TText('A' + RemovableFatDrive);
	else if (aDriveType==EDriveNand)
		name2[0] = TText('A' + NandDrive);
	else 
		name2[0] = TText('A' + InternalRemovableFatDrive);
	name2.Append( KFragmentedFileName2 );
	RFile file2;
	r = file2.Create(TheFs, name2, EFileWrite);
	test_KErrNone(r);
	file2.Close();
	TInt64 randomSeed;
	TBuf8<KMaxFragmentSize> tempBuf;	
	TUint8 *buf;
	TInt fileSize = 0;
	TInt fragmentSize;
	TInt randomLength;
	TInt pos1;
	TInt pos2;
	TInt mycount = 0;
	do	
		{
		fragmentSize = 0;
		pos1 = 0;
		pos2 = 0;
		buf = (TUint8*) tempBuf.Ptr();
		tempBuf.Zero();
		randomLength = Math::Random() % KMaxFragmentSize;
		randomSeed = Math::Random();
		tempBuf.SetLength(randomLength);

		while (randomLength-- && fragmentSize++ < KMaxFragmentSize && fileSize++ < KMaxFileSize)
			{
			*buf++ = (TUint8)('A' + (Math::Rand(randomSeed) % ('Z' - 'A')));
			}
		r = file1.Open( TheFs, name1, EFileWrite );
		test_KErrNone(r);
		r = file1.Seek( ESeekEnd, pos1 );
		test_KErrNone(r);
		r = file1.Write( pos1, tempBuf );
		test_KErrNone(r);
		r = file1.Flush();
		test_KErrNone(r);		
		file1.Close();
		if ( mycount++ < 6 )
			{
			r = file2.Open( TheFs, name2, EFileWrite );
			test_KErrNone(r);
			r = file2.Seek( ESeekEnd, pos2 );
			test_KErrNone(r);
			r = file2.Write( pos2, tempBuf );
			test_KErrNone(r);
			r = file2.Flush();
			test_KErrNone(r);
			file2.Close();
			}
		} while ( fileSize < KMaxFileSize );
	CFileMan* fMan=CFileMan::NewL(TheFs);
	test(fMan!=NULL);
	r = fMan->Delete(name2);
	test_KErrNone(r);
	delete fMan;
	}

LOCAL_C void FindDrive(DriveType aDriveType)
	{
	TInt i;
	TInt c = 0;
	for( i = EDriveA; i < EDriveZ; i++ )
		{
		TDriveInfo info;
		TInt r = TheFs.Drive(info, i);
		if ( r != KErrNone )
			continue;
		test_KErrNone(r);
		if ( aDriveType == EDriveNand )	
			{
			c++ == 0 ? test.Printf( _L("Searching for NAND drive.")) : test.Printf( _L("."));
			if ( info.iType == EMediaNANDFlash && ((info.iMediaAtt & KMediaAttWriteProtected) == 0) )
				{
				if ( info.iDriveAtt & KDriveAttPageable )
					Pageable = ETrue;
				NandDrive = i;
				test.Printf( _L("Found NAND drive: %d\n"), NandDrive );
				break;
				}
			}
		else if ( aDriveType == EDriveRam )
			{
			c++ == 0 ? test.Printf( _L("Searching for RAM FAT drive.")) : test.Printf( _L("."));
			if ( (info.iType == EMediaRam) && ( info.iDriveAtt == (KDriveAttLocal|KDriveAttInternal) ) && ( info.iMediaAtt == (KMediaAttVariableSize|KMediaAttFormattable) ) )
				{
				if ( info.iDriveAtt & KDriveAttPageable )
					Pageable = ETrue;
				RamFatDrive = i;
				test.Printf( _L("Found RAM FAT drive: %d\n"), RamFatDrive );
				break;
				}
			}
		else if ( aDriveType == EDriveRemovable )
			{
			c++ == 0 ? test.Printf( _L("Searching for removable FAT drive.")) : test.Printf( _L("."));
			if ( info.iType == EMediaHardDisk && ( info.iDriveAtt == (KDriveAttLocal|KDriveAttRemovable) ) )
				{
				if ( info.iDriveAtt & KDriveAttPageable )
					Pageable = ETrue;
				RemovableFatDrive = i;
				test.Printf( _L("Found removable FAT drive: %d\n"), RemovableFatDrive );
				break;
				}
			}
		else if ( aDriveType == EDriveInternalRemovable )
			{
			c++ == 0 ? test.Printf( _L("Searching for internal removable FAT drive.")) : test.Printf( _L("."));
			if ( info.iType == EMediaHardDisk && ( info.iDriveAtt == (KDriveAttLocal|KDriveAttInternal) ) )
				{
				if ( info.iDriveAtt & KDriveAttPageable )
					Pageable = ETrue;
				InternalRemovableFatDrive = i;
				test.Printf( _L("Found internal removable FAT drive: %d\n"), InternalRemovableFatDrive );
				break;
				}
			}
		}
		if ( i == EDriveZ )
			{
			switch(aDriveType)
				{
				case EDriveNand:
					test.Printf( _L("NAND drive not found.\n") );
					break;
				case EDriveRam:
					test.Printf( _L("RAM FAT drive not found.\n") );
					break;
				case EDriveRemovable:
					test.Printf( _L("Removable FAT drive not found.\n") );
					break;
				case EDriveInternalRemovable:
					test.Printf( _L("Internal removable FAT drive not found.\n") );
					break;
				default:
					test.Printf( _L("Drive not found.\n") );
				}			
			}
	}

//************************
// Entry point

GLDEF_C void CallTestsL(void)
	{
	test.Title(); 
	test.Start( _L("BlockMap Test\n") );

	TInt testFileSize = 0;
	RFile testFile;
	TInt r = testFile.Open(TheFs, KTestFile, EFileRead);
	test_KErrNone(r);
	r = testFile.Size(testFileSize);
	test_KErrNone(r);
	test(testFileSize>16384);
	testFile.Close();

	if ( gDriveToTest == 'C' )
		{
		TInt value;
		r = HAL::Get( HAL::EMachineUid, value );
		test_KErrNone(r);
		if ( value != HAL::EMachineUid_Lubbock )	// Lubbock cannot run FindDrive as it doesn't support the NAND API
			{
			test.Next(_L("Test BlockMap retrieval on NAND FAT."));
			FindDrive(EDriveNand);
			if ( NandDrive > -1 )	// not finding a NAND drive isn't an error as only NAND builds have one
				{
				r = TestBlockMapNandFATUserData(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(1024, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(1020, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(1024, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(1020, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(1025, 1200);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(0, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(-5, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(-5, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(testFileSize, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(2000, 2001);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFATUserData(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(2000, 2000);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFATUserData(2048, 2048);
				test_Value(r, r == KErrArgument );
				test.Printf(_L("Generating Fragmented File..."));
				GenerateFragmentedFiles(EDriveNand);
				test.Printf(_L("Done!\n"));
				test.Next(_L("Test BlockMap retrieval on NAND FAT (User area) (fragmented)."));	
				r = TestBlockMapFragmented(EDriveNand, 0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 1024, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 1020, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 1024, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 1020, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 1025, 1200);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 0, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveNand, -5, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveNand, -5, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveNand, 0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveNand, testFileSize, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveNand, 0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 2000, 2001);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveNand, 0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveNand, 2000, 2000);
				test_Value(r, r == KErrArgument );
				Finished = ETrue;
				r = TestBlockMapFragmented(EDriveNand, 2048, 2048);
				test_Value(r, r == KErrArgument );
				test.Next(_L("Test BlockMap retrieval on NAND FAT."));	
				r = TestBlockMapNandFAT(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(1024, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(1020, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(1024, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(1020, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(1025, 1200);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(0, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(-5, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(-5, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(testFileSize, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(2000, 2001);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandFAT(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(2000, 2000);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandFAT(2048, 2048);
				test_Value(r, r == KErrArgument );
				test.Next(_L("Test BlockMap retrieval on NAND ROFS."));
				r = TestBlockMapNandROFS(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(1024, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(1020, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(1024, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(1020, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(1025, 1200);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(0, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(-5, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(-5, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(testFileSize, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(2000, 2001);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapNandROFS(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(2000, 2000);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapNandROFS(2048, 2048);
				test_Value(r, r == KErrArgument );
				test.Next(_L("Test BlockMap retrieval on RAM FAT."));
				FindDrive(EDriveRam);
				test( RamFatDrive > -1 );
				r = TestBlockMapRamFAT(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(1024, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(1020, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(1024, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(1020, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(1025, 1200);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(0, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(-5, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(-5, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(testFileSize, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(2000, 2001);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapRamFAT(0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(2000, 2000);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapRamFAT(2048, 2048);
				test_Value(r, r == KErrArgument ); 
				test.Next(_L("Test BlockMap retrieval on Ram FAT (2)."));
				r = TestBlockMapRamFAT2(0, -1);
				test_Value(r, r == KErrNotSupported );
				FindDrive(EDriveRemovable);
				if ( RemovableFatDrive > -1)
					{
					test.Next(_L("Test BlockMap retrieval on removable FAT."));
					r = TestBlockMapRemovableFAT(0, -1);
					test_Value(r, r == Pageable ? KErrNotSupported : KErrCompletion);
					}
				else
					{
					test.Next(_L("Test BlockMap retrieval on internal removable FAT."));
					FindDrive(EDriveInternalRemovable);
					test( InternalRemovableFatDrive > -1);
					r = TestBlockMapInternalRemovableFAT(0, -1);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(1024, 4096);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(1020, 4100);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(1024, 4100);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(1020, 4096);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(1025, 1200);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(0, testFileSize+100);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(-5, -1);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(-5, testFileSize+100);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(0, 0);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(testFileSize, -1);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(0, -1);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(2000, 2001);
					test_Value(r, r == KErrCompletion );
					r = TestBlockMapInternalRemovableFAT(0, 0);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(2000, 2000);
					test_Value(r, r == KErrArgument );
					r = TestBlockMapInternalRemovableFAT(2048, 2048);
					test_Value(r, r == KErrArgument );
					}
				test.Next(_L("Test BlockMap retrieval on Ram FAT (fragmented)."));	
				test.Printf(_L("Generating Fragmented File..."));
				GenerateFragmentedFiles(EDriveRam);
				test.Printf(_L("Done!\n"));
				Finished = EFalse;
				r = TestBlockMapFragmented(EDriveRam, 0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 1020, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 2049, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 1024, 4100);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 1020, 4096);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 1025, 1200);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 0, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveRam, -5, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveRam, -5, testFileSize+100);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveRam, 0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveRam, testFileSize, -1);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveRam, 0, -1);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 2000, 2001);
				test_Value(r, r == KErrCompletion );
				r = TestBlockMapFragmented(EDriveRam, 0, 0);
				test_Value(r, r == KErrArgument );
				r = TestBlockMapFragmented(EDriveRam, 2000, 2000);
				test_Value(r, r == KErrArgument );
				Finished = ETrue;
				r = TestBlockMapFragmented(EDriveRam, 2048, 2048);
				test_Value(r, r == KErrArgument );
				}
			else
				{
				test.Printf( _L("NAND drive not found, skipping test.\n") );
				}
			}
		}
	test.End();
	test.Close();
	}
