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
// Main entry point for the tests
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <f32file.h>
#include <e32test.h>


#if defined(_DEBUG)
#define __PRINT(t) {RDebug::Print(t);}
#define __PRINT1(t,a) {RDebug::Print(t,a);}
#define __PRINT2(t,a,b) {RDebug::Print(t,a,b);}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#endif

GLREF_C void DoTestL(TInt aDriveToTest);

GLREF_D RTest	test;

//_LIT( KRofsFilesystemName, "Rofs" ); // Comment out warning

GLDEF_D RFs TheFs;
GLDEF_D TChar gDriveToTest;

void ParseCommandArguments()
//
//
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	test.Printf(_L("CLP=%S"),&token);
	if(token.Length()!=0)		
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else
		{
#ifdef __WINS__
		gDriveToTest='V';
#else
		gDriveToTest='J';
#endif
		}
	}

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	ParseCommandArguments();

	TInt r=TheFs.Connect();
	__PRINT1(_L("Connect ret %d"),r);
	
	TInt theDrive;
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
	__PRINT1(_L("Look up drive number returned %d"),r);
	
	if( KErrNone == r )
		{
		TRAP( r, DoTestL(theDrive) );
		}

    TheFs.Close();

	if( KErrNone != r )
		{
		test.Printf( _L("Failed with error %d\n"), r );
		}

#ifdef __WINS__
// The sin of sins for tests
//	test.Printf( _L("Press a key...") );
//	test.Getch();
#endif	
	delete cleanup;
    return r;
	}

