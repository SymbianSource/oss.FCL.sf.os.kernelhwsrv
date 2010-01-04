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



#include "t_fat32server.h"
#include "t_fat32base.h"
#include "t_fat32format.h"
#include "t_fat32mount.h"
#include "t_fat32readraw.h"
#include "t_fat32writeraw.h"
#include "t_fat32readfile.h"
#include "t_fat32writefile.h"
#include "t_fat32checkdisk.h"
#include "t_fat32calculate.h"
#include "t_fat32filldisk.h"
#include "t_fat32clusterbound.h"
#include "t_fat32readwrite.h"
#include "t_fat32larger512.h"

CBaseTestFat32Server* CBaseTestFat32Server::NewL()
	{
	CBaseTestFat32Server* server = new (ELeave) CBaseTestFat32Server();
	CleanupStack::PushL(server);
	RProcess handle = RProcess();
	TParsePtrC serverName(handle.FileName());
	server->ConstructL(serverName.Name());
	CleanupStack::Pop(server);
	return server;
	}

LOCAL_C void MainL()
	{
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);

	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CBaseTestFat32Server* server = NULL;
	TRAPD(err,server = CBaseTestFat32Server::NewL());
	if(!err)
		{
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAP_IGNORE(MainL());
	delete cleanup;
	return KErrNone;
    }

CTestStep* CBaseTestFat32Server::CreateTestStep(const TDesC& aStepName)
	{
	CTestStep* testStep = NULL;
	
	if (aStepName == KTestStepFormat)
		{
		testStep = new CBaseTestFat32Format();
		}
		else if (aStepName == KTestStepMount)
		{
		testStep = new CBaseTestFat32Mount();
		}
		else if (aStepName == KTestStepReadRaw)
		{
		testStep = new CBaseTestFat32ReadRaw();
		}
		else if (aStepName == KTestStepWriteRaw)
		{
		testStep = new CBaseTestFat32WriteRaw();
		}
		else if (aStepName == KTestStepReadFile)
		{
		testStep = new CBaseTestFat32ReadFile();
		}
		else if (aStepName == KTestStepWriteFile)
		{
		testStep = new CBaseTestFat32WriteFile();
		}
		else if (aStepName == KTestStepCheckDisk)
		{
		testStep = new CBaseTestFat32CheckDisk();
		}
		else if (aStepName == KTestStepCalculate)
		{
		testStep = new CBaseTestFat32Calculate();
		}
		else if (aStepName == KTestStepFillDisk)
		{
		testStep = new CBaseTestFat32FillDisk();			
		}
		else if (aStepName == KTestStepClusterBound)
		{
		testStep = new CBaseTestFat32ClusterBound();			
		}
		else if (aStepName == KTestStepReadWrite)
		{
		testStep = new CBaseTestFat32ReadWrite();			
		}
		else if (aStepName == KTestStepLarger512)
		{
		testStep = new CBaseTestFat32Larger512();			
		}
	return testStep;
	}
