// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test traversal of directories and locating files in directories
// 
//

#include <e32std.h>
#include <e32std_private.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <f32file.h>
#include "utl.h"

GLREF_D RFs TheFs;

RTest	test( _L("T_ROFSDIR") );

_LIT( KTestFile1, "root.txt" );
_LIT( KTestFile2, "Dir1\\level1.txt" );
_LIT( KTestFile3, "Dir1\\Dir2\\level2.txt" );
_LIT( KTestFile4, "Dir1\\Dir2\\Dir3\\level3.txt" );
_LIT( KTestFile5, "Dir1\\Dir2\\Dir3\\Dir4\\level4.txt" );
_LIT( KTestFile6, "Dir1\\Dir2\\Dir3\\Dir4\\Dir5\\level5.txt" );
_LIT( KTestFile7, "Dir1\\Dir2\\Dir3\\Dir4\\Dir5\\Dir6\\level6.txt" );
_LIT( KTestFile8, "Dir1\\Dir2\\Dir3\\Dir4\\Dir5\\Dir6\\Dir7\\level7.txt" );
_LIT( KTestFile9, "DeepDir1\\DeepDir2\\DeepDir3\\DeepDir4\\DeepDir5\\DeepDir6\\DeepDir7\\DeepDir8\\DeepDir9\\DeepDir10\\DeepDir11\\DeepDir12\\DeepDir13\\DeepDir14\\file.txt" );
_LIT( KTestFile10, "Parent\\parfile.txt" );
_LIT( KTestFile11, "Parent\\SubDir1A\\subfileA.txt" );
_LIT( KTestFile12, "Parent\\SubDir1B\\subfileB.txt" );
_LIT( KTestFile13, "Parent\\SubDir1C\\subfileC.txt" );
_LIT( KTestFile14, "Parent\\SubDir1D\\subfileD.txt" );
_LIT( KTestFile15, "Parent\\SubDir1E\\subfileE.txt" );
_LIT( KTestFile16, "Parent\\SubDir1F\\SubSubA\\subsub_a.txt" );
_LIT( KTestFile17, "Parent\\SubDir1F\\SubSubB\\subsub_b.txt" );
_LIT( KTestFile18, "Parent\\SubDir1F\\SubSubC\\subsub_c.txt" );
_LIT( KTestFile19, "Parent\\SubDir1F\\SubSubD\\subsub_d.txt" );
_LIT( KTestFile20, "Mixed\\par1.txt" );
_LIT( KTestFile21, "Mixed\\SubDir1\\sub1.txt" );
_LIT( KTestFile22, "Mixed\\SubDir2\\sub2.txt" );
_LIT( KTestFile23, "Mixed\\SubDir3\\sub3.txt" );
_LIT( KTestFile24, "Mixed\\SubDir4\\sub4.txt" );
_LIT( KTestFile25, "Mixed\\SubDir5\\sub5.txt" );
_LIT( KTestFile26, "Mixed\\SubDir6\\sub6.txt" );
_LIT( KTestFile27, "Mixed\\SubDir7\\sub7.txt" );
_LIT( KTestFile28, "Mixed\\SubDir8\\sub8.txt" );
_LIT( KTestFile29, "Mixed\\par2.txt" );
_LIT( KTestFile30, "Mixed\\par3.txt" );
_LIT( KTestFile31, "Mixed\\par4.txt" );
_LIT( KTestFile32, "Mixed\\par5.txt" );
_LIT( KTestFile33, "Mixed\\par6.txt" );
_LIT( KTestFile34, "Mixed\\par7.txt" );
_LIT( KTestFile35, "Mixed\\par8.txt" );
// required to test extension
_LIT( KTestFile36, "ext.txt" );
_LIT( KTestFile37, "Dir1\\ext.txt" );
_LIT( KTestFile38, "Dir1\\level1_ext.txt" );
//required to test the unique files 
//(Note: These file names depend on the mount ids. Hence change in mount order will affect the test)
_LIT( KTestFile39, "Exattrib\\test1.txt[02-00]" );
_LIT( KTestFile40, "Exattrib\\test1.txt[03-00]");
_LIT( KTestFile41, "Exattrib\\test2.txt[03-00]" );
_LIT( KTestFile42, "Exattrib\\test3-1.txt[02-00]");
_LIT( KTestFile43, "Exattrib\\test5-1.txt[x-y][03-00]");

const TInt KRootDirEntryCount = 17;

_LIT( KRootFile, "root.txt" );
_LIT( KRootFileExt, "ext.txt" );
_LIT( KRootDir1, "Dir1" );
_LIT( KRootDeepDir1, "DeepDir1" );
_LIT( KRootDirParent, "Parent" );
_LIT( KRootDirMixed, "Mixed" );
_LIT( KRootDirReadTest, "ReadTest" );
_LIT( KRootDirAttrib, "Attrib" );
_LIT( KRootDirExattrib, "Exattrib" );
_LIT( KRootDirResource, "Resource" );
_LIT( KRootImg, "Img" );
_LIT( KRootSys, "sys" );
_LIT( KRootSystem, "system" );
_LIT( KRootTest, "Test" );
_LIT( KRootDirMultiple, "Multiple" );
_LIT( KRootAutoexec, "autoexec.bat" );

_LIT( KRootDirScripts, "scripts" );


_LIT( KSubDirOnlyBase, "Parent\\SubDir1F\\" );
_LIT( KSubSubPattern, "SubSub%c" );
const TInt KSubDirOnlyCount = 4;

_LIT( KFilesOnlyBase, "Parent\\SubDir1F\\SubSubA\\" );
_LIT( KSubSubFilePattern, "subsub_%c.txt" );
const TInt KFilesOnlyCount = 1;

_LIT( KMixedDirBase, "Mixed\\" );
_LIT( KMixedDirPattern, "SubDir%d" );
_LIT( KMixedFilePattern, "par%d.txt" );
const TInt KMixedSubDirCount = 8;
const TInt KMixedSubFileCount = 8;


const TInt KMultipleDirEntryCount=14;
_LIT( KMultipleFile1, "new1.txt" );
_LIT( KMultipleFile2, "new2.txt" );
_LIT( KMultipleFile3, "new3.txt" );
_LIT( KMultipleFile4, "replaceme1.txt" );
_LIT( KMultipleFile5, "replaceme2.txt" );
_LIT( KMultipleFile6, "replaceme3.txt" );
_LIT( KMultipleFile7, "replaceme4.txt" );
_LIT( KMultipleFile8, "rom.txt" );
_LIT( KMultipleFile9, "romreplace.txt" );
// required to test multiple rofs
_LIT( KMultipleFile10, "multirofs.txt" );
// files on user data fat partition
_LIT( KMultipleFile11, "new4.txt" );
_LIT( KMultipleFile12, "replaceme5.txt" );
_LIT( KMultipleFile13, "romreplacefat.txt" );
_LIT( KMultipleFile14, "t_file.cpp" );

const TInt KExattribDirEntryCount = 11;

_LIT( KExattribFile1, "test1.txt[02-00]" );
_LIT( KExattribFile2, "test1.txt[03-00]" );
_LIT( KExattribFile3, "test2.txt[03-00]" );
_LIT( KExattribFile4, "test3-1.txt[02-00]" );
_LIT( KExattribFile5, "test4-2.txt[03-00]" );
_LIT( KExattribFile6, "test5-1.txt[x-y][03-00]" );
_LIT( KExattribFile7, "test5-2.txt[x-y]" );
_LIT( KExattribFile8, "test8-1.txt[04-00]" );
_LIT( KExattribFile9, "test6-1.txt[04-00]" );
_LIT( KExattribFile10, "test7-1.txt[03-00]" );
_LIT( KExattribFile11, "test7-1.txt[04-00]" );

_LIT( KDriveBase, " :\\" );
_LIT( KDriveMultiple, "Multiple\\" );
_LIT( KDriveExattrib, "Exattrib\\" );
_LIT( KWildCard, "*" );


enum TEntrySet
	{
	ERoot,
	ERootExtension,
	EMultiple,
	EExattrib,
	};

LOCAL_C void TestOpenFilesL(TInt aDriveToTest, TBool aExtension, TBool aMultipleRofs)
//
// Tests that we can open files in various directories on the drive
//
	{
	TInt index = 35;
	if(aExtension)
		index=37;
	if(aMultipleRofs)
		index=42;

	const TDesC* const fileArray[43] =
		{
		&KTestFile1, &KTestFile2, &KTestFile3, &KTestFile4, &KTestFile5,
		&KTestFile6, &KTestFile7, &KTestFile8, &KTestFile9, &KTestFile10,
		&KTestFile11, &KTestFile12, &KTestFile13, &KTestFile14, &KTestFile15,
		&KTestFile16, &KTestFile17, &KTestFile18, &KTestFile19, &KTestFile20,
		&KTestFile21, &KTestFile22, &KTestFile23, &KTestFile24, &KTestFile25,
		&KTestFile26, &KTestFile27, &KTestFile28, &KTestFile29, &KTestFile30,
		&KTestFile31, &KTestFile32, &KTestFile33, &KTestFile34, &KTestFile35,
		&KTestFile36, &KTestFile37, &KTestFile38, &KTestFile39, &KTestFile40,
		&KTestFile41, &KTestFile42, &KTestFile43
		};

	test.Next( _L("Test opening files in directories") );
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	for( TInt i = 0; i < index; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		if(aExtension && (i==1))
			name.Append( *fileArray[i+36] );
		else
			name.Append( *fileArray[i] );
		test.Printf( _L("Opening file %S\n"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );
		file.Close();
		}
	}


LOCAL_C void TestScanDirL(TInt aDriveToTest, TEntrySet aEntrySet)
	//
	// Scan the root directory file
	//
	{
	const TDesC* const rootEntriesExt[ KRootDirEntryCount ] = 
		{
		&KRootFileExt, &KRootFile, &KRootDir1, &KRootDeepDir1, 
		&KRootDirParent, &KRootDirMixed, &KRootDirReadTest, &KRootDirAttrib, &KRootDirExattrib, 
		&KRootDirResource, &KRootImg, &KRootSys, &KRootSystem, &KRootTest, &KRootDirMultiple,
		&KRootDirScripts,
		&KRootAutoexec
		};

	const TDesC* const rootEntries[ KRootDirEntryCount-1 ] = 
		{
		&KRootFile, &KRootDir1, &KRootDeepDir1, 
		&KRootDirParent, &KRootDirMixed, &KRootDirReadTest, &KRootDirAttrib, &KRootDirExattrib, 
		&KRootDirResource, &KRootImg, &KRootSys, &KRootSystem, &KRootTest, &KRootDirMultiple, 
		&KRootDirScripts,
		&KRootAutoexec
		};

	const TDesC* const multipleEntries[ KMultipleDirEntryCount ] =
		{
		&KMultipleFile1, &KMultipleFile2, &KMultipleFile3, &KMultipleFile4, &KMultipleFile5,
		&KMultipleFile6, &KMultipleFile7, &KMultipleFile8, &KMultipleFile9, &KMultipleFile10,
		&KMultipleFile11, &KMultipleFile12, &KMultipleFile13, &KMultipleFile14
		};

	const TDesC* const exattribEntries[ KExattribDirEntryCount ] =
		{
		&KExattribFile1, &KExattribFile2, &KExattribFile3, &KExattribFile4,
		&KExattribFile5, &KExattribFile6, &KExattribFile7, &KExattribFile8,
		&KExattribFile9, &KExattribFile10, &KExattribFile11
		};

	const TDesC* const *entries = NULL;
	TInt DirCount=0;
	TBool seen[KRootDirEntryCount];
	memclr(seen, KRootDirEntryCount*sizeof(TBool));
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	
	switch (aEntrySet)
		{
		case ERoot:
			DirCount=KRootDirEntryCount-1;
			entries=rootEntries;
			test.Next( _L("Scanning root directory") );
		break;
		case ERootExtension:
			DirCount=KRootDirEntryCount;
			entries=rootEntriesExt;
			test.Next( _L("Scanning root extension directory") );
		break;
		case EMultiple:
			DirCount=KMultipleDirEntryCount;
			entries=multipleEntries;
			name.Append(KDriveMultiple);
			test.Next( _L("Scanning multiple directory") );
		break;
		case EExattrib:
			DirCount=KExattribDirEntryCount;
			entries=exattribEntries;
			name.Append(KDriveExattrib);
			test.Next( _L("Scanning Exattrib directory") );
		break;
		default:
			test(EFalse);
		}

	CDir* dir;
	TInt r = TheFs.GetDir( name, KEntryAttMaskSupported, ESortNone, dir );
	TEST_FOR_ERROR( r );
	
	TInt actualCount = dir->Count();
	for(TInt i = 0; i < actualCount; i++)
		{
		const TEntry& e = dir->operator[](i);
		TFileName lowerName(e.iName);
		lowerName.LowerCase();
		TBool found = EFalse;
		// try to find the name in our list
		for(TInt j = 0; j < DirCount && !found; j++)
			{
			TFileName expectedLower(*entries[j]);
			expectedLower.LowerCase();
			// Check the directory name is that expected.
			found = lowerName == expectedLower;
			if(found)
				{
				if(seen[j])
					{
					test.Printf( _L("ERROR: saw entry %S twice\n"), &e.iName );
					test( EFalse );
					}
				else
					{
					seen[j] = ETrue;
					}
				}
			}
		if (!found)
			{
			test.Printf( _L("ERROR: entry %S unknown\n"), &e.iName );
			test( EFalse );
			}
		}
	// All the entries are those expected but are there enough matching entries.
	// Allow the autoexe.bat to not be present so the test can pass when run 
	// when not part of an auto test rom.
	TInt tolerance = 1;
	if (aEntrySet == ERoot || aEntrySet == ERootExtension)
		{// The "scripts" directory is not present on all roms.
		tolerance++;
		}
	test_Value(actualCount, actualCount >= DirCount-tolerance);
	delete dir;
	}



LOCAL_C void TestSubDirsOnlyL(TInt aDriveToTest)
	//
	// Tests that scanning a directory containing only subdirectories
	// only returns entries marked as directories
	//
	{
	test.Next( _L("Testing scan of dir containing only subdirs") );
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KSubDirOnlyBase );
	name.Append( KWildCard );

	TInt r = TheFs.GetDir( name, KEntryAttMaskSupported, ESortNone, dir );
	TEST_FOR_ERROR( r );
	
	TEST_FOR_MATCH( dir->Count(), KSubDirOnlyCount );

	TBuf<32> subEntryName;
	for( TInt i = 0; i < KSubDirOnlyCount; i++ )
		{
		const TEntry& e = dir->operator[](i);
		subEntryName.Format( KSubSubPattern, 'A' + i );
		test.Printf( _L("Found entry %S\n"), &e.iName );
		test( e.iName == subEntryName );
		test( e.IsDir() );
		}
	delete dir;
	}


LOCAL_C void TestFilesOnlyL(TInt aDriveToTest)
	//
	// Tests that scanning a directory containing only files
	// does not return any entries marked as files
	//
	{
	test.Next( _L("Testing scan of dir containing only files") );
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KFilesOnlyBase );
	name.Append( KWildCard );

	TInt r = TheFs.GetDir( name, KEntryAttMaskSupported, ESortNone, dir );
	TEST_FOR_ERROR( r );
	
	TEST_FOR_MATCH( dir->Count(), KFilesOnlyCount );

	TBuf<32> subEntryName;
	for( TInt i = 0; i < KFilesOnlyCount; i++ )
		{
		const TEntry& e = dir->operator[](i);
		subEntryName.Format( KSubSubFilePattern, 'a' + i );
		test.Printf( _L("Found entry %S\n"), &e.iName );
		test( e.iName == subEntryName );
		test( !e.IsDir() );
		}
	delete dir;
	}


LOCAL_C void TestMaskDirsL(TInt aDriveToTest)
	//
	// Test scanning a directory with masking to return only directories
	//
	{
	test.Next( _L("Testing scan of dir masking out files") );
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KMixedDirBase );
	name.Append( KWildCard );

	TInt r = TheFs.GetDir( name, KEntryAttMatchExclusive | KEntryAttDir, ESortNone, dir );
	TEST_FOR_ERROR( r );
	
	TEST_FOR_MATCH( dir->Count(), KMixedSubDirCount );

	TBuf<32> subEntryName;
	for( TInt i = 0; i < KMixedSubDirCount; i++ )
		{
		const TEntry& e = dir->operator[](i);
		subEntryName.Format( KMixedDirPattern, i+1 );
		test.Printf( _L("Found entry %S\n"), &e.iName );
		test( e.iName == subEntryName );
		test( e.IsDir() );
		}
	delete dir;
	}

LOCAL_C void TestMaskFilesL(TInt aDriveToTest)
	//
	// Test scanning a directory with masking to return only files
	//
	{
	test.Next( _L("Testing scan of dir masking out directories") );
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KMixedDirBase );
	name.Append( KWildCard );

	TInt r = TheFs.GetDir( name, KEntryAttMatchExclude | KEntryAttDir, ESortNone, dir );
	TEST_FOR_ERROR( r );
	
	TEST_FOR_MATCH( dir->Count(), KMixedSubFileCount );

	TBuf<32> subEntryName;
	for( TInt i = 0; i < KMixedSubFileCount; i++ )
		{
		const TEntry& e = dir->operator[](i);
		subEntryName.Format( KMixedFilePattern, i+1 );
		test.Printf( _L("Found entry %S\n"), &e.iName );
		test( e.iName == subEntryName );
		test( !e.IsDir() );
		}
	delete dir;
	}

LOCAL_C void TestGetDirL(TInt aDriveToTest)
//
// Test UID scanning of a directory
//
	{
	test.Next( _L("Testing scan of dir with UID bitmask") );

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KRootDirResource );
	name.Append(_L("\\"));

	const TUid KUidInterfaceImplementationCollectionInfo = {0x101F747D};
	
	TUidType rscUidType(KNullUid,KUidInterfaceImplementationCollectionInfo,KNullUid);

	CDir* dir;
	TInt r = TheFs.GetDir(name, rscUidType, ESortByUid, dir);
	test(r==KErrNone);
	TInt count = dir->Count();
	test(count==10);
	delete dir;

	TUidType uidType2(TUid::Uid('X'),TUid::Uid('Y'),TUid::Uid('Z'));
	r = TheFs.GetDir(name, uidType2, ESortByUid, dir);
	test(r==KErrNone);
	count = dir->Count();
	test(count==1);
	delete dir;
	}



//************************
// Entry point

void DoTestL(TInt aDriveToTest)
	{
	test.Title();
	test.Start( _L("Testing ROFS directory structure") );

	test.Printf( _L("Looking for ROFS extension..\n"));
	TBool extension = EFalse;
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	name.SetLength( 3 );	// trim back to drive specifier
	name.Append( KTestFile36 );

	RFile file;
	test.Printf( _L("Attempt to open file %S.."), &name );
	TInt r = file.Open( TheFs, name, EFileRead );
	if(r==KErrNone)
		{
		extension=ETrue;
		file.Close();
		test.Printf( _L("ROFS extension found.\n"));
		}
	else if(r==KErrNotFound)
		{
		test.Printf( _L("Not found, ROFS extension not present.\n"));
		}

	test.Printf( _L("Looking for multiple ROFS..\n"));
	TBool multipleRofs = EFalse;
	name.SetLength( 3 );	// trim back to drive specifier
	name.Append( KMultipleFile10 );

	test.Printf( _L("Attempt to open file %S.."), &name );
	r = file.Open( TheFs, name, EFileRead );
	if(r==KErrNone)
		{
		multipleRofs=ETrue;
		file.Close();
		test.Printf( _L("Multiple ROFS found. %S is present.\n"), &name);
		}
	else if(r==KErrNotFound)
		{
		test.Printf( _L("No multiple ROFS found. %S is not present.\n"), &name);
		}
	
	TestOpenFilesL(aDriveToTest,extension,multipleRofs);
	TestScanDirL(aDriveToTest, extension?ERootExtension:ERoot);
	TestSubDirsOnlyL(aDriveToTest);
	TestFilesOnlyL(aDriveToTest);
	TestMaskDirsL(aDriveToTest);
	TestMaskFilesL(aDriveToTest);
	TestGetDirL(aDriveToTest);

	// Test dir works for muliple rofs
	if (multipleRofs)
	{
		TestScanDirL(aDriveToTest, EMultiple);
		TestScanDirL(aDriveToTest, EExattrib);
	}

	test.End();
	}
