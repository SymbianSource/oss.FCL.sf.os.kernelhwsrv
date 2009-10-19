// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Thindump is a small tool for dumping text files.  The text file 
// given will be dumped using RDebug::Print meaning the text goes to 
// both the console and the Debug Serial Port.
// 
//

/**
 @file 
 @internalComponent
*/

#include <e32test.h>
#include <f32file.h>
#ifndef SYMBIAN_BASEDUMP
#include <s32file.h>
#include <c32comm.h>
#include <bacline.h>
#endif // SYMBIAN_BASEDUMP


GLDEF_D RTest gTest(_L("thindump utility"));
GLDEF_D RFs gFs;

LOCAL_C void SendFileL(const TPtrC &aFilename, const TInt &aPause)
/**
Dump narrow text file to console, comms & debug (WINS)
*/
	{
	TInt err;

	// Open file and determine size
	RFile file;
	err = file.Open(gFs, aFilename, EFileRead);
	if( err == KErrNotFound || err == KErrBadName )
		{
		gTest.Printf(_L("File %S does not exist"), &aFilename);
		return;	
		}
	else if( err == KErrInUse ) // Test Execute opens with ShareAny
		{
		gTest.Printf(_L("Open failed as %S is in use.  Retry open with EFileShareAny"), &aFilename);
		err = file.Open(gFs, aFilename, EFileRead|EFileShareAny);
		}

//	gTest.Printf(_L("SendFileL Line %d, RFile::Open err = %d"), __LINE__, err);
//	gTest.Getch();
	User::LeaveIfError(err);

	TInt size;
	err = file.Size(size);
//	gTest.Printf(_L("SendFileL Line %d, RFile::Size err = %d"), __LINE__, err);
//	gTest.Getch();
	User::LeaveIfError(err);

	gTest.Printf(_L("File %S (%d bytes)"), &aFilename, size);

	// Get ready to read file
	const TInt KBufferSize = 32;
	const TInt KBufferMaxLength = 1024;

	_LIT(KCharLF, "\x0a");
	_LIT(KCharCR, "\x0d");

	TInt pos = 0;
	TInt lfPos;
	TInt crPos;

	TBuf8<KBufferSize+1> buf8(KBufferSize+1);
	TBuf<KBufferSize+1> buf16(KBufferSize+1);

	HBufC* format16=HBufC::NewLC(KBufferMaxLength);

	// Read block from file
	while(pos < size)
		{
		err = file.Read(buf8, KBufferSize);
//		gTest.Printf(_L("SendFileL Line %d, RFile::Read err = %d"), __LINE__, err);
//		gTest.Getch();
		User::LeaveIfError(err);

		// Expand to 16 bit chars
		buf16.Copy(buf8);

		// Remove carriage returns
		while ( (crPos = buf16.Find(KCharCR)) != KErrNotFound )
			buf16.Delete(crPos, 1);

		// Find line feeds
		TPtr ptr16( format16->Des() );
		while ( (lfPos = buf16.Find(KCharLF)) != KErrNotFound )
			{
			// Extract this line & append to any buffered line
			TPtrC ptrLeft = buf16.Left(lfPos+1);
			ptr16.Append(ptrLeft);
			buf16.Delete(0, lfPos+1);

			// Print this line
			gTest.Printf(_L("%S"), &ptr16 );
			ptr16.Zero();

			// Small 10ms pause to prevent data loss during upload
			User::After(10000);
			}

		ptr16.Append(buf16);
		pos += KBufferSize;
		}

	// Output any partial line still in buffer
	if (format16->Length())
		{
		TPtr ptr16( format16->Des() );
		gTest.Printf(_L("%S"), &ptr16 );
		}

	CleanupStack::PopAndDestroy(1, format16);

  	if (aPause)
  		{
  		gTest.Printf(_L("Dump complete. Press any key ..."));
  		gTest.Getch();
  		}

	file.Close();
	}


LOCAL_C void InitGlobalsL()
/**
Initialise global variables.
*/
	{
	TInt err;

	err = gFs.Connect();
//	gTest.Printf(_L("InitGlobals Line %d, RFs::Connect err = %d"), __LINE__, err);
//	gTest.Getch();
	User::LeaveIfError(err);
	}


LOCAL_C void DestroyGlobals()
/**
Free global variables
*/
	{
	gFs.Close();
	}


LOCAL_C void RunSendFileL()
/**
Transmit the file down the Debug Port
*/
	{
	InitGlobalsL();

	TInt pause=EFalse;
	_LIT(KOptionNoPause,"-nop");
	_LIT(KOptionPause,"-p");
	_LIT(KDumpFileDefault, "c:\\log.txt");

	// Obtain command line parameters
	TPtrC filename( KDumpFileDefault );

#ifndef SYMBIAN_BASEDUMP
	CCommandLineArguments* args = CCommandLineArguments::NewLC();

	for(TInt i=1;i<args->Count();i++)
		{
		if(args->Arg(i).MatchF(KOptionNoPause)==0)
			{
			pause=EFalse;
			}
		else if(args->Arg(i).MatchF(KOptionPause)==0)
			{
			pause=ETrue;
			}
		else
			{
			filename.Set(args->Arg(i));
			}
		}
#else
	TBuf<256> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);

	while (!lex.Eos())
		{
		TPtrC token;
		token.Set(lex.NextToken());
		if (token.Length()==0)
			{
			break;	// ignore trailing whitespace
			}
		else if (token==KOptionNoPause)
			{
			pause=EFalse;
			}
		else if (token==KOptionPause)
			{
			pause=ETrue;
			}
		else
			{
			filename.Set(token);
			}
		}
#endif
	SendFileL( filename, pause );

#ifndef SYMBIAN_BASEDUMP
	CleanupStack::PopAndDestroy(1, args);
#endif
	DestroyGlobals();
	}


EXPORT_C TInt E32Main()
/**
Main Program
*/
    {
	CTrapCleanup* cleanup = CTrapCleanup::New();
	CActiveScheduler* theActiveScheduler = new CActiveScheduler();
	CActiveScheduler::Install(theActiveScheduler);

	__UHEAP_MARK;

	User::ResetInactivityTime();

	gTest.Printf(_L("========== Start Log File =========="));

	TRAPD(err,RunSendFileL());
	if (err!=KErrNone)
		{
		gTest.Printf(_L("thindump left with Error No %d"), err);
		}

	gTest.Printf(_L("========== Finish Log File =========="));

	gTest.Close();

	__UHEAP_MARKEND;

	delete cleanup;
	delete theActiveScheduler;

	return KErrNone;
    }
