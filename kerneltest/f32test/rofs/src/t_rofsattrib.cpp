// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test attribute settings on files
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <f32file.h>
#include "utl.h"



GLREF_D RFs TheFs;

RTest	test( _L("T_ROFSATTRIB") );

_LIT( KFileRsh1, "rsh" );
_LIT( KFileRs1, "rs" );
_LIT( KFileRh1, "rh" );
_LIT( KFileSh1, "sh" );
_LIT( KFileS1, "s" );
_LIT( KFileH1, "h" );
_LIT( KFileR1, "r" );
_LIT( KFileR2, "r2" );
_LIT( KFileR3, "r3" );
_LIT( KFileS2, "s2" );
_LIT( KFileS3, "s3" );
_LIT( KFileH2, "h2" );
_LIT( KFileH3, "h3" );
_LIT( KFileRs2, "rs2" );
_LIT( KFileRs3, "rs3" );
_LIT( KFileRh2, "rh2" );
_LIT( KFileRh3, "rh3" );
_LIT( KFileRsh2, "rsh2" );
_LIT( KFileRsh3, "rsh3" );

_LIT( KDirectoryBase, "Attrib\\");

LOCAL_D const TUint KFileListAllAttribs[] =
	{
	KEntryAttReadOnly | KEntryAttSystem | KEntryAttHidden,	// KFileRsh1
	KEntryAttReadOnly | KEntryAttSystem,	// KFileRs1
	KEntryAttReadOnly | KEntryAttHidden,	// KFileRh1
	KEntryAttSystem	| KEntryAttHidden,// KFileSh1
	KEntryAttSystem,	// KFileS1
	KEntryAttHidden,	// KFileH1
	KEntryAttReadOnly,	// KFileR1
	KEntryAttReadOnly,	// KFileR2
	KEntryAttReadOnly,	// KFileR3
	KEntryAttSystem,	// KFileS2
	KEntryAttSystem,	// KFileS3
	KEntryAttHidden,	// KFileH2
	KEntryAttHidden,	// KFileH3
	KEntryAttReadOnly | KEntryAttSystem,	// KFileRs2
	KEntryAttReadOnly | KEntryAttSystem,	// KFileRs3
	KEntryAttReadOnly | KEntryAttHidden,	// KFileRh2
	KEntryAttReadOnly | KEntryAttHidden,	// KFileRh3
	KEntryAttReadOnly | KEntryAttSystem | KEntryAttHidden,	// KFileRsh2
	KEntryAttReadOnly | KEntryAttSystem | KEntryAttHidden,	// KFileRsh3
	};

LOCAL_D const TDesC* KFileListAll[] =
	{
	&KFileRsh1,
	&KFileRs1,
	&KFileRh1,
	&KFileSh1,
	&KFileS1,
	&KFileH1,
	&KFileR1,
	&KFileR2,
	&KFileR3,
	&KFileS2,
	&KFileS3,
	&KFileH2,
	&KFileH3,
	&KFileRs2,
	&KFileRs3,
	&KFileRh2,
	&KFileRh3,
	&KFileRsh2,
	&KFileRsh3,
	NULL
	};

LOCAL_D const TDesC* KFileListReadOnly[] =
	{
	&KFileRsh1,
	&KFileRs1,
	&KFileRh1,
	&KFileR1,
	&KFileR2,
	&KFileR3,
	&KFileRs2,
	&KFileRs3,
	&KFileRh2,
	&KFileRh3,
	&KFileRsh2,
	&KFileRsh3,
	NULL
	};

LOCAL_D const TDesC* KFileListNotHidden[] =
	{
	&KFileRs1,
	&KFileS1,
	&KFileR1,
	&KFileR2,
	&KFileR3,
	&KFileS2,
	&KFileS3,
	&KFileRs2,
	&KFileRs3,
	NULL
	};


LOCAL_D const TDesC* KFileListNotSystem[] =
	{
	&KFileRh1,
	&KFileH1,
	&KFileR1,
	&KFileR2,
	&KFileR3,
	&KFileH2,
	&KFileH3,
	&KFileRh2,
	&KFileRh3,
	NULL
	};


LOCAL_D const TDesC* KFileListNotSystemAndHidden[] =
	{
	&KFileRsh1,
	&KFileRs1,
	&KFileRh1,
	&KFileS1,
	&KFileH1,
	&KFileR1,
	&KFileR2,
	&KFileR3,
	&KFileS2,
	&KFileS3,
	&KFileH2,
	&KFileH3,
	&KFileRs2,
	&KFileRs3,
	&KFileRh2,
	&KFileRh3,
	&KFileRsh2,
	&KFileRsh3,
	NULL
	};


LOCAL_D const TDesC* KFileListNotSystemOrHidden[] =
	{
	&KFileR1,
	&KFileR2,
	&KFileR3,
	NULL
	};



_LIT( KDriveBase, " :\\" );
_LIT( KWildCard, "*" );


LOCAL_C void TestFileAttribsL(TInt aDriveToTest)
	{
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KDirectoryBase );
	name.Append( KWildCard );

	TInt r = TheFs.GetDir( name, KEntryAttMatchMask, ESortNone, dir );
	TEST_FOR_ERROR( r );
	test( dir->Count() > 0 );

	TInt i;
	TEntry e;
	for( i = 0; KFileListAll[i]; i++ )
		{
		e = dir->operator[](i);
		test.Printf( _L("Found entry %S"), &e.iName );
		test( e.iName == *KFileListAll[i] );

		// check attributes
		TEST_FOR_MATCH( e.iAtt, KFileListAllAttribs[i] );
		}
	TEST_FOR_MATCH( i, dir->Count() );
	delete dir;
	}


LOCAL_C void TestScanFilesL( const TDesC* aFileList[], TUint aAttribMask, TInt aDriveToTest )
	//
	// Tests scanning a directory with attribute mask
	//
	{
	CDir* dir;

	TFileName name(KDriveBase);
	name[0] = TText('A' + aDriveToTest);
	name.Append( KDirectoryBase );
	name.Append( KWildCard );

	TInt r = TheFs.GetDir( name, aAttribMask, ESortNone, dir );
	TEST_FOR_ERROR( r );

	test( dir->Count() > 0 );

	TInt i;
	TEntry e;
	for( i = 0; aFileList[i]; i++ )
		{
		e = dir->operator[](i);
		test.Printf( _L("Found entry %S"), &e.iName );
		test( e.iName == *aFileList[i] );
		}
	TEST_FOR_MATCH( i, dir->Count() );
	delete dir;
	}





//************************
// Entry point

void DoTestL(TInt aDriveToTest)
	{
	test.Title();
	test.Start( _L("Testing ROFS directory structure") );
	
	const TUint KNotSystem = KEntryAttReadOnly | KEntryAttHidden;
	const TUint KNotHidden = KEntryAttReadOnly | KEntryAttSystem;
	const TUint KNotSystemOrHidden = KEntryAttNormal;

	test.Next( _L("Checking attributes of files") );
	TestFileAttribsL(aDriveToTest);
	test.Next( _L("Testing not system") );
	TestScanFilesL( KFileListNotSystem, KNotSystem, aDriveToTest );
	test.Next( _L("Testing not hidden") );
	TestScanFilesL( KFileListNotHidden, KNotHidden, aDriveToTest );
	test.Next( _L("Testing not system or hidden") );
	TestScanFilesL( KFileListNotSystemOrHidden, KNotSystemOrHidden, aDriveToTest );
	test.End();
	}
