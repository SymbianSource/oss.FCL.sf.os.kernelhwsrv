// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test opening of hidden, replaced and newly added files belonging to multiple ROFS.
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <f32file.h>
#include "utl.h"

GLREF_D RFs TheFs;

RTest test( _L("T_ROFSMULTIPLE") );

// Required to test multiple ROFS
_LIT( KTestHidden1, "Multiple\\hidden1.txt" );
_LIT( KTestHidden2, "Multiple\\hidden2.txt" );
_LIT( KTestHidden3, "Multiple\\hidden3.txt" );
_LIT( KTestNew1, "Multiple\\new1.txt" );
_LIT( KTestNew2, "Multiple\\new2.txt" );
_LIT( KTestNew3, "Multiple\\new3.txt" );
_LIT( KTestNew4, "Multiple\\new4.txt" );
_LIT( KTestReplaceMe1, "Multiple\\replaceme1.txt" );
_LIT( KTestReplaceMe2, "Multiple\\replaceme2.txt" );
_LIT( KTestReplaceMe3, "Multiple\\replaceme3.txt" );
_LIT( KTestReplaceMe4, "Multiple\\replaceme4.txt" );
_LIT( KTestReplaceMe5, "Multiple\\replaceme5.txt" );
_LIT( KTestRom,        "Multiple\\rom.txt" );
_LIT( KTestRomHide,    "Multiple\\romhide.txt" );
_LIT( KTestRomReplace, "Multiple\\romreplace.txt" );
_LIT( KTestRomReplaceFat, "Multiple\\romreplacefat.txt" );
_LIT8( KReplaceMe8FileContent1, "rofs1" );
_LIT8( KReplaceMe8FileContent2, "rofs2" );
_LIT8( KReplaceMe8FileContent3, "rofs3" );
_LIT8( KReplaceMe8FileContent5, "cfat5" );


_LIT( KRootFileMultiple, "Multiple\\multirofs.txt");

_LIT( KDriveBase, " :\\" );

const TInt KNewlyAddedFilesCount = 5;
const TDesC* const newlyAddedFilesArray[5] =
	{
	&KTestNew1, &KTestNew2, &KTestNew3, &KTestNew4, &KTestRom
	};
	
const TInt KReplacedFilesCount = 7;
const TDesC* const replacedFilesArray[7] =
	{
	&KTestReplaceMe1, &KTestReplaceMe2, &KTestReplaceMe3, &KTestReplaceMe4, &KTestRomReplace, &KTestReplaceMe5,  &KTestRomReplaceFat
	};

const TInt KHiddenFilesCount = 4;
const TDesC* const hiddenFilesArray[4] =
	{
	&KTestHidden1, &KTestHidden2, &KTestHidden3, &KTestRomHide 
	};

LOCAL_C void TestMultipleRofsL(TInt aDriveToTest)
//
// Test multiple Rofs
//
	{
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	TInt i;
	// hidden
	test.Next( _L("Test opening hidden files.") );
	for( i = 0; i < KHiddenFilesCount; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		name.Append( *hiddenFilesArray[i] );
		test.Printf( _L("Opening file %S\n"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_MATCH( r, KErrNotFound );
		file.Close();
		}

	// newly added
	test.Next( _L("Test opening newly added files.") );
	for( i = 0; i < KNewlyAddedFilesCount; i++ )
		{
		name.SetLength( 3 );
		name.Append( *newlyAddedFilesArray[i] );
		test.Printf( _L("Opening file %S\n"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );
		file.Close();
		}
	
	// replaced	
	test.Next( _L("Test opening replaced files.") );
	for( i = 0; i < KReplacedFilesCount; i++ )
		{
		name.SetLength( 3 );
		name.Append( *replacedFilesArray[i] );
		test.Printf( _L("Opening file %S\n"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );
		TBuf8<5> buf;
		r = file.Read( buf );
		TEST_FOR_ERROR( r );
		if ( i == 0 )
			test(buf == KReplaceMe8FileContent2);
		else if (i<4) // i == 1 -> 3
			test(buf == KReplaceMe8FileContent3);
		else if (i==4)
			test(buf == KReplaceMe8FileContent1);
		else 
			test(buf == KReplaceMe8FileContent5);			
		file.Close();
		}
	}

LOCAL_C void TestFilesInRomL(TInt aDriveToTest)
//
// Test whether files in ROFS appear to be in ROM area of Z:
//
	{
	test.Next( _L("Test if file is in ROM area of Z:") );	
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	TInt i;

	for( i = 0; i < KNewlyAddedFilesCount; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		name.Append( *newlyAddedFilesArray[i] );
		test.Printf( _L("Testing newly added file %S\n"), &name );
		
		
		if (i==KNewlyAddedFilesCount)
			test( NULL != TheFs.IsFileInRom( name ) );
		else
			test( NULL == TheFs.IsFileInRom( name ) );
		}

	test.Next( _L("Test if replaced file is in ROM area of Z:") );	

	for( i = 0; i < KReplacedFilesCount; i++ )
		{
		name.SetLength( 3 );
		name.Append( *replacedFilesArray[i] );
		test.Printf( _L("Testing replaced file %S\n"), &name );
		test( NULL == TheFs.IsFileInRom( name ) );
		}

	test.Next( _L("Test if hidden file is in ROM area of Z:") );

	for( i = 0; i < KHiddenFilesCount; i++ )
		{
		name.SetLength( 3 );
		name.Append( *hiddenFilesArray[i] );
		test.Printf( _L("Testing hidden file %S\n"), &name );
		test( NULL == TheFs.IsFileInRom( name ) );
		}	
	}

LOCAL_C void TestReadFileSectionL(TInt aDriveToTest)
//
//	Test reading data from a file without opening it.
//
	{
	test.Next( _L("Testing ReadFileSection()."));	
	
	TBuf8<12> testDes;
	TInt r;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	TInt i;
	for( i = 0; i < KNewlyAddedFilesCount; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		name.Append( *newlyAddedFilesArray[i] );
		r=TheFs.ReadFileSection(name,0,testDes,3);
		test(r==KErrNone);
		test(testDes.Length()==3);
		test(testDes==_L8("hel"));

		name.SetLength( 3 );
		name.Append( *newlyAddedFilesArray[i] );
		r=TheFs.ReadFileSection(name,0,testDes,4);
		test(r==KErrNone);
		test(testDes.Length()==4);
		test(testDes==_L8("hell"));

		name.SetLength( 3 );
		name.Append( *newlyAddedFilesArray[i] );
		r=TheFs.ReadFileSection(name,1,testDes,4);
		test(r==KErrNone);
		test(testDes.Length()==4);
		test(testDes==_L8("ello"));
		}

	test.Next( _L("Testing ReadFileSection() on replaced files."));	

	for( i = 0; i < KReplacedFilesCount; i++ )
		{
		name.SetLength( 3 );
		name.Append( *replacedFilesArray[i] );
		r=TheFs.ReadFileSection(name,4,testDes,1);
		test(r==KErrNone);
		test(testDes.Length()==1);
		if ( i == 0 )
			test(testDes==_L8("2"));
		else if (i<4)
			test(testDes==_L8("3"));
		else if (i==4)
			test(testDes==_L8("1"));
		else 
			test(testDes==_L8("5"));
		}

	test.Next( _L("Testing ReadFileSection() on hidden files."));	

	for( i = 0; i < KHiddenFilesCount; i++ )
		{	
		name.SetLength( 3 );
		name.Append( *hiddenFilesArray[i] );
		r=TheFs.ReadFileSection(name,0,testDes,1);
		test(r==KErrNotFound);
		}
	}

LOCAL_C void TestEntryL(TInt aDriveToTest)
//
//	Test accessing the entry details for a hidden file.
//
	{
	test.Next( _L("Test accessing the entry details for replaced files.") );
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	TEntry entry;
	TInt r;

	TTime dirtimes[4] = { TDateTime(2005,EDecember,9,2,0,3,0),		// 10/12/2005
						TDateTime(2005,EDecember,14,4,30,33,0),		// 15/12/2005
						TDateTime(2006,EJanuary,1,15,45,37,0), 		// 02/01/2006
						TDateTime(2006,EAugust,10,17,47,04,0) }; 	// 11/08/2006

	TInt i;
	for( i = 0; i < KReplacedFilesCount; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier	
		name.Append( *replacedFilesArray[i] );
		r = TheFs.Entry(name, entry);
		test(r==KErrNone);
		test (entry.iName==replacedFilesArray[i]->Right(entry.iName.Length()));

		if ( i == 0 )
			test(entry.iModified==dirtimes[1]);
		else if (i<4)
			test(entry.iModified==dirtimes[2]);
		else if (i==4)
			test(entry.iModified==dirtimes[0]);
		else
			test(entry.iModified==dirtimes[3]);
		}
	
	test.Next( _L("Test accessing the entry details for hidden files.") );

	for ( i = 0; i < KHiddenFilesCount; i++ )
		{	
		name.SetLength( 3 );
		name.Append( *hiddenFilesArray[i] );
		r = TheFs.Entry(name, entry);
		test(r==KErrNotFound);
		}
	}

//************************
// Entry point

void DoTestL(TInt aDriveToTest)
	{
	test.Title();
	test.Start( _L("Testing opening hidden, replaced and newly added files belonging to multiple ROFS.") );

	test.Printf( _L("Looking for multiple ROFS..\n"));
	TBool multipleRofs = EFalse;
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	name.SetLength( 3 );	// trim back to drive specifier
	name.Append( KRootFileMultiple );

	RFile file;
	test.Printf( _L("Attempt to open file %S..\n"), &name );
	TInt r = file.Open( TheFs, name, EFileRead );
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
	
	if (multipleRofs)
		{	
		TestMultipleRofsL(aDriveToTest);
		TestFilesInRomL(aDriveToTest);
		TestReadFileSectionL(aDriveToTest);
		TestEntryL(aDriveToTest);
		}
	test.End();
	}
