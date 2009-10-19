// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\t_fir.cpp
// 
//

#include "t_fir.h"

void RunAppL(void)
	{
	// Construct and install the active scheduler
	CActiveScheduler *exampleScheduler = new (ELeave) CActiveScheduler();
	// Push onto the cleanup stack
	CleanupStack::PushL(exampleScheduler);
	// Install as the active scheduler
	CActiveScheduler::Install(exampleScheduler); 
	//Create Obex objects and console handler
	CConsoleBase* aConsole = 
	  Console::NewL(_L("Fir Test Code"),TSize(KConsFullScreen,KConsFullScreen));//KDefaultConsWidth,KDefaultConsHeight			  
	CleanupStack::PushL(aConsole);
	CActiveConsole* my_console = CActiveConsole::NewLC(aConsole);
	CleanupStack::PushL(my_console);
	my_console->RequestCharacter();
	CActiveScheduler::Start();
	User::After(2000000);
	CleanupStack::PopAndDestroy(3);
	}

TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New(); // get clean-up stack
	TRAPD(error,RunAppL()); // more initialization, then do example
	__ASSERT_ALWAYS(!error,User::Panic(_L("EPOC32EX"),error));
	delete cleanup; // destroy clean-up stack
	__UHEAP_MARKEND;

	return 0; // and return
	}
