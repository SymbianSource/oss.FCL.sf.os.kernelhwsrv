// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file 
 @internalComponent
*/

#include <e32base.h>
#include <e32base_private.h>
#include <e32std.h>
#include <e32std_private.h>

#include "t_otgdi_fdfactor.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "t_otgdi_fdfactor_mainTraces.h"
#endif

LOCAL_C void DoStartL()
	{
	_LIT(KDriverLddFileName,"usbhubdriver");
	TInt err = User::LoadLogicalDevice(KDriverLddFileName);
	if(err)
		{
		OstTrace1(TRACE_NORMAL, DOSTARTL_DOSTARTL, "FDFActor failed to load LDD %d",err);
		}

	// Create active scheduler (to run active objects)
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);

	TUsbBusObserver myBusObserver;	//	DS TODO Has null implementations of notification functions
	
	//	Object to watch for attachments, suspending devices soon after they are attached
	CActorFDF* myFdfActor=CActorFDF::NewL(myBusObserver);
	CleanupStack::PushL(myFdfActor);
	
	//	Object to watch for a rendezvous with the t_otgdi.exe process 
	//	(indicating no further need for FDF Actor)
	//	Stops the active scheduler when t_otgdi.exe signals rendezvous
	CFdfTOtgdiWatcher* myCFdfTOtgdiWatcher = CFdfTOtgdiWatcher::NewL();
	CleanupStack::PushL(myCFdfTOtgdiWatcher);
	
	myFdfActor->Monitor();
	//	Signal back to t_otgdi.exe that we're up and running, ready to deal with device attach/detach
	RProcess::Rendezvous(KErrNone);
	
	CActiveScheduler::Start();

	// Delete active scheduler
	CleanupStack::PopAndDestroy(3, scheduler);
	
	err = User::FreeLogicalDevice(RUsbHubDriver::Name());
	if(err)
		{
		OstTrace1(TRACE_NORMAL, DOSTARTL_DOSTARTL_DUP01, "FDFActor failed to unload LDD %d",err);
		}	
	}


//  Global Functions

GLDEF_C TInt E32Main()
	{
	// Create cleanup stack
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();

	TRAPD(mainError,DoStartL());


	if (mainError)
		{
		OstTrace1(TRACE_NORMAL, E32MAIN_E32MAIN, "FDF Actor left with %d", mainError);
		//	Also means that we left before we did a Rendezvous(KErrNone) to free up t_otgdi.exe
		//	Rendezvous with KErrAbort to indicate we couldn't start up properly.
		RProcess::Rendezvous(KErrAbort);
		}

	delete cleanup;
	__UHEAP_MARKEND;

	OstTrace0(TRACE_NORMAL, E32MAIN_E32MAIN_DUP01, "About to end FDFActor process");
	return KErrNone;
	}



