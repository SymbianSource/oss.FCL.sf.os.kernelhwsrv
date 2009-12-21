/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


#include <e32base.h>
#include <e32cons.h>
#include <e32std.h>

/*@{*/
const TInt KDefaultInterval			= 5000000;
_LIT(KPromptPressAnyKey,			"And press any key to continue...");
_LIT(KPromptConsole,				"TEF");
_LIT(KPromptMMCMessage,				"After hitting a key to continue, eject and insert the MMC card (press F5 and release on the emulator).\n");
/*@}*/


LOCAL_C void MainL()
/**
 * Secure variant, contain the prompt console
 */
	{
	CConsoleBase 	*console = Console::NewL(KPromptConsole, 
		TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);

    console->Printf(KPromptMMCMessage);
	console->Printf(KPromptPressAnyKey);

	TRequestStatus	timerStatus;
	RTimer			timer;
	CleanupClosePushL(timer);
	timer.CreateLocal();
	timer.After(timerStatus, TTimeIntervalMicroSeconds32(KDefaultInterval));

	TRequestStatus	readStatus;
	console->Read(readStatus);

	User::WaitForRequest(timerStatus, readStatus);
	if ( timerStatus!=KErrNone )
		{
		timer.Cancel();
		User::WaitForRequest(timerStatus);
		}
	if ( readStatus==KRequestPending )
		{
		console->ReadCancel();
		User::WaitForRequest(readStatus);
		}

	CleanupStack::PopAndDestroy(2, console);
	}


GLDEF_C TInt E32Main()
/**
 * @return - Standard Epoc error code on process exit
 * Secure variant only
 * Process entry point. Called by client using RProcess API
 */
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		RProcess::Rendezvous(KErrNoMemory);
		return KErrNoMemory;
		}
#if (defined TRAP_IGNORE)
	TRAP_IGNORE(MainL());
	RProcess::Rendezvous(KErrNone);
#else
	TRAPD(err,MainL());
	RProcess::Rendezvous(err);
#endif

	delete cleanup;
	return KErrNone;
    }
