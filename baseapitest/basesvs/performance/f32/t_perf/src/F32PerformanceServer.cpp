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


#include "F32PerformanceServer.h"
#include "EntryStep.h"
#include "ReadFileStep.h"
#include "SeekFileStep.h"
#include "WriteFileStep.h"
#include "UtilsCleanupStep.h"
#include "UtilsSetupStep.h"



// Function : CT_F32PerformanceServer
// Description :CT_F32PerformanceServer class constructor			 
// @return :CT_F32PerformanceServer		
CT_F32PerformanceServer::CT_F32PerformanceServer()
:CTestServer ()
	{
	
	}


// Function : NewL
// @return	:An instance of test server 
// Description: Same code for Secure and non-secure variants
// Called inside the MainL() function to create and start the
// CTestServer derived server.

CT_F32PerformanceServer* 	CT_F32PerformanceServer::NewL()
	{
	CT_F32PerformanceServer* performServer=new(ELeave) 	CT_F32PerformanceServer();
	CleanupStack::PushL(performServer);
	performServer->ConstructL(KT_PerformServ);
	CleanupStack::Pop(performServer);
	return performServer;
	}

// Function : ~CT_F32PerformanceServer
// Description :CT_F32PerformanceServer class destructor	
CT_F32PerformanceServer::~CT_F32PerformanceServer()
	{
	
	}


// Function : MainL
// Description:	Secure variant
// 				Much simpler, uses the new Rendezvous() 
//				call to sync with the client	
LOCAL_C void MainL()
	{
	// Leave the hooks in for platform security
	#if (defined __DATA_CAGING__)
		RProcess().DataCaging(RProcess::EDataCagingOn);
		RProcess().SecureApi(RProcess::ESecureApiOn);
	#endif

	CActiveScheduler*		performScheduler=NULL;
	performScheduler=new(ELeave) CActiveScheduler;
	//Install me as current ActiveScheduler
	CActiveScheduler::Install(performScheduler);
	CT_F32PerformanceServer*	server = NULL;
	
	// Create CT_ F32PerformanceServer(the CTestServer derived server)
	TRAPD(err,server = CT_F32PerformanceServer::NewL());
	if(!err)
		{
		// Sync with the client and enter this active scheduler
		RProcess::Rendezvous(KErrNone);
		performScheduler->Start();
		}
		
	delete server;
	delete 	performScheduler;
	
	}
//End MainL
	

// Function : E32Main
// @return  Standard Epoc error code on exit
GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
		
	#if (defined TRAP_IGNORE)
		TRAP_IGNORE(MainL());
	#else
		TRAPD(err,MainL());
	#endif
	
	delete cleanup;
	return KErrNone;
	
    }
	

// @return - A CTestStep derived instance
// Description : Implementation of CTestServer pure virtual
//  (i.e Creates the test steps) 
//  @param :	aStepName	The name of the test step to be created
CTestStep* CT_F32PerformanceServer::CreateTestStep(const TDesC& aStepName)
	{
	CTestStep*	testStep = NULL;
	if (aStepName == KT_EntryStep)
		{
		testStep = new CT_EntryStep();
		}
	else if (aStepName ==KT_ReadFileStep)
		{
		testStep = new CT_ReadFileStep ();
		}	
	else if (aStepName==KT_SeekFileStep)
		{
		testStep= new  CT_SeekFileStep ();
		}			
	else if (aStepName==KT_WriteFileStep)
		{
		testStep= new CT_WriteFileStep ();
		}
	else if(aStepName==KT_CleanupStep)
		{
		testStep=new CT_CleanupStep();
		}
	else if (aStepName==KT_SetupStep)
		{
		testStep=new CT_SetupStep();
		}	
	return testStep;	
	}

