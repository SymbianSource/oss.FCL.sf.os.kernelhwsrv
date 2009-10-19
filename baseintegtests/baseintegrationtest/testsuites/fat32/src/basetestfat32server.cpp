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
//

#include "basetestfat32server.h"
#include "basetestfat32base.h"
#include "basetestfat32format.h"
#include "basetestfat32mount.h"
#include "basetestfat32readraw.h"
#include "basetestfat32writeraw.h"
#include "basetestfat32readfile.h"
#include "basetestfat32writefile.h"
#include "basetestfat32checkdisk.h"
#include "basetestfat32calculate.h"
#include "basetestfat32filldisk.h"
#include "basetestfat32clusterbound.h"
#include "basetestfat32readwrite.h"
#include "basetestfat32larger512.h"

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
