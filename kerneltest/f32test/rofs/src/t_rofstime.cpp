// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Tests timestamp on files
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <f32file.h>
#include "utl.h"

GLREF_D RFs TheFs;

RTest	test( _L("T_ROFSTIME") );



_LIT( KTestFile1, "root.txt" );
_LIT( KTestFile2, "Mixed\\par1.txt" );
_LIT( KTestFile3, "Mixed\\SubDir1\\sub1.txt" );
_LIT( KTestFile4, "Mixed\\SubDir2\\sub2.txt" );
_LIT( KTestFile5, "Mixed\\SubDir3\\sub3.txt" );
_LIT( KTestFile6, "Mixed\\SubDir4\\sub4.txt" );
_LIT( KTestFile7, "Mixed\\SubDir5\\sub5.txt" );
_LIT( KTestFile8, "Mixed\\SubDir6\\sub6.txt" );
_LIT( KTestFile9, "Mixed\\SubDir7\\sub7.txt" );
_LIT( KTestFile10, "Mixed\\SubDir8\\sub8.txt" );
_LIT( KTestFile11, "Mixed\\par2.txt" );
_LIT( KTestFile12, "Mixed\\par3.txt" );
_LIT( KTestFile13, "Mixed\\par4.txt" );
_LIT( KTestFile14, "Mixed\\par5.txt" );
_LIT( KTestFile15, "Mixed\\par6.txt" );
_LIT( KTestFile16, "Mixed\\par7.txt" );
_LIT( KTestFile17, "Mixed\\par8.txt" );
_LIT( KTestFile18, "ext.txt" );

_LIT( KDriveBase, " :\\" );
_LIT( KDirectoryScanPath, "Mixed\\*" );

//_LIT( KTimeStamp, "23/11/2001 6:44:07" );

const TInt KYear = 2001;
const TMonth KMonth = ENovember;
const TInt KDay = 23 - 1;
const TInt KHour = 6;
const TInt KMinute = 44;
const TInt KSecond = 7;

const TInt KYearExt = 2005;
const TMonth KMonthExt = EJune;
const TInt KDayExt = 14 - 1;
const TInt KHourExt = 20;
const TInt KMinuteExt = 12;
const TInt KSecondExt = 20;

TTime gTime;


const TDesC* const gTestFileArray[18] =
		{
		&KTestFile1, &KTestFile2, &KTestFile3, &KTestFile4, &KTestFile5,
		&KTestFile6, &KTestFile7, &KTestFile8, &KTestFile9, &KTestFile10,
		&KTestFile11, &KTestFile12, &KTestFile13, &KTestFile14, &KTestFile15,
		&KTestFile16, &KTestFile17, &KTestFile18
		};



LOCAL_C void TestRFileL(TInt aDriveToTest, TBool aExtension)
//
// Tests that file modified timestamp is correct
//
	{

	test.Next( _L("Test file modified time") );
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	TInt index =17;
	if (aExtension) index =18;
	for( TInt i = 0; i < index; i++ )
		{
		name.SetLength( 3 );	// trim back to drive specifier
		name.Append( *gTestFileArray[i] );
		test.Printf( _L("Opening file %S"), &name );
		RFile file;
		TInt r = file.Open( TheFs, name, EFileRead );
		TEST_FOR_ERROR( r );
		TTime mod;
		r = file.Modified( mod );
		TEST_FOR_ERROR( r );

		if( mod != gTime )
			{
			TDateTime expected = gTime.DateTime();
			TDateTime got = mod.DateTime();
			test.Printf(_L("Times don't match, expected %d-%d-%d %d:%d:%d, read %d-%d-%d %d:%d:%d\n"), 
				expected.Day()+1,
				expected.Month()+1,
				expected.Year(),
				expected.Hour(),
				expected.Minute(),
				expected.Second(),
				got.Day()+1,
				got.Month()+1,
				got.Year(),
				got.Hour(),
				got.Minute(),
				got.Second() );
			test.operator()( EFalse, __LINE__, (TText*)__FILE__ );
			}
		file.Close();
		}
	}




LOCAL_C void TestDirScanL(TInt aDriveToTest)
	//
	// Tests that scanning a directory gives correct timestamp for each entry
	//
	{
	test.Next( _L("Testing directory scan") );
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KDirectoryScanPath );

	TInt r = TheFs.GetDir( name, KEntryAttMaskSupported, ESortNone, dir );
	TEST_FOR_ERROR( r );
	
	for( TInt i = dir->Count() - 1; i > 0; i-- )
		{
		const TEntry& e = dir->operator[](i);
		if( e.iModified != gTime )
			{
			TDateTime expected = gTime.DateTime();
			TDateTime got = (e.iModified).DateTime();
			test.Printf(_L("Times don't match, expected %d-%d-%d %d:%d:%d, read %d-%d-%d %d:%d:%d\n"), 
				expected.Day()+1,
				expected.Month()+1,
				expected.Year(),
				expected.Hour(),
				expected.Minute(),
				expected.Second(),
				got.Day()+1,
				got.Month()+1,
				got.Year(),
				got.Hour(),
				got.Minute(),
				got.Second() );
			test.operator()( EFalse, __LINE__, (TText*)__FILE__ );
			}
		}
	delete dir;
	}



//************************
// Entry point

void DoTestL(TInt aDriveToTest)
	{
	TDateTime dateTime( KYear, KMonth, KDay, KHour, KMinute, KSecond, 0 );

	test.Title();
	test.Start( _L("Testing ROFS timestamp") );

	test.Printf( _L("Looking for ROFS extension\n"));
	TBool extension = EFalse;
	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);

	name.SetLength( 3 );	// trim back to drive specifier
	name.Append( KTestFile18 );

	RFile file;
	test.Printf( _L("Opening file %S"), &name );
	TInt r = file.Open( TheFs, name, EFileRead );
	if(r==KErrNone)
		{
		extension=ETrue;
		file.Close();
		dateTime.Set(KYearExt, KMonthExt, KDayExt, KHourExt, KMinuteExt, KSecondExt, 0 );
		test.Printf( _L("ROFS extension found\n"));
		}
	else if(r==KErrNotFound)
		{
		test.Printf( _L("Not found, ROFS extension not present\n"));
		}
	
	gTime = TTime( dateTime );
	
	TestRFileL(aDriveToTest, extension);
	TestDirScanL(aDriveToTest);

	test.End();
	}
