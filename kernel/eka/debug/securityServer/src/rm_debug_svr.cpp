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
// Entry point to debug security server, sets up server/session
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#include <e32base.h>
#include <e32base_private.h>
#include <rm_debug_api.h>

#include "c_security_svr_server.h"
#include "c_security_svr_session.h"
#include "rm_debug_logging.h"

using namespace Debug;

/**
Perform all server initialisation, in particular creation of the
scheduler and server and then run the scheduler
*/
void RunServerL()
	{
	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> new(ELeave) CActiveScheduler\n" );
	CActiveScheduler* s=new(ELeave) CActiveScheduler;

	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> CleanupStack::PushL(s)\n" );
	CleanupStack::PushL(s);

	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> CActiveScheduler::Install()\n" );
	CActiveScheduler::Install(s);

	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> CSecuritySvrServer::NewLC()\n" );
	CSecuritySvrServer::NewLC();

	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> Rendezvous(KErrNone)\n" );
	// Signal whoever has started us that we have done so.
	RProcess::Rendezvous(KErrNone);
	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : <- Rendezvous()\n" );

	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> CActiveScheduler::Start()\n" );
	CActiveScheduler::Start();
	LOG_MSG( "rm_debug_svr.cpp::RunServerL() <- CActiveScheduler::Start()\n" );

	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : -> CleanupStack::PopAndDestroy()\n" );
	CleanupStack::PopAndDestroy(2, s);
	LOG_MSG( "rm_debug_svr.cpp::RunServerL() : <- CleanupStack::PopAndDestroy()\n" );
	}

/**
Entry point for debug security server
*/
GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New();
	TInt r = KErrNoMemory;
	if (cleanup)
		{
		TRAP(r,RunServerL());
		delete cleanup;
		}
	__UHEAP_MARKEND;
	return r;
	}
