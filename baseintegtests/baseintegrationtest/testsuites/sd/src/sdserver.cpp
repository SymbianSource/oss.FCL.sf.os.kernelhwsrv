// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "sdserver.h"
#include "sdbase.h"
#include "sdbigfileread.h"
#include "sdbigfilewrite.h"
#include "sdcheckdisk.h"
#include "sdcopyfile.h"
#include "sddialogbox.h"
#include "sddisplayinfo.h"
#include "sdfieldcheck.h"
#include "sdfileoperations1.h"
#include "sdfileoperations2.h"
#include "sdfillsectors.h"
#include "sdformat.h"
#include "sdreadfiles1.h"
#include "sdreadfiles2.h"
#include "sdusb.h"

/*
Constructor. Initialise shareable data.

@param None
@return None
*/
TBaseTestSDSharedData::TBaseTestSDSharedData()
	{
	iPartitionBootSector = iTotalSector = iFsType = iNumberOfClusters = iSectorsPerFat = iReservedSectorCount = 0;
	}

/*
Create a new instance of a Test Server

@param None
@return A pointer to the new Test Server
*/
CBaseTestSDServer* CBaseTestSDServer::NewL()
	{
	CBaseTestSDServer* server = new (ELeave) CBaseTestSDServer();
	CleanupStack::PushL(server);
	RProcess handle = RProcess();
	TParsePtrC serverName(handle.FileName());
	server->ConstructL(serverName.Name());
	CleanupStack::Pop(server);
	return server;
	}

/*
Constructor. Initialise shareable data.

@param None
@return None
*/
LOCAL_C void MainL()
	{
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);

	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	CBaseTestSDServer* server = NULL;
	TRAPD(err,server = CBaseTestSDServer::NewL());
	if(!err)
		{
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}

/*
Server Entry Point

@param None
@return KErrNone if successful or KErrNoMemory if there is not enough memory.
*/
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

/*
Create a new Test Step object.

@param aStepName The name of the Test Step to be created
@return A pointer to the new Test Step, or NULL if unsuccesful.
*/
CTestStep* CBaseTestSDServer::CreateTestStep(const TDesC& aStepName)
	{
	CTestStep* testStep = NULL;
	
	if (aStepName == KTestStepBigFileRead)
		{
		testStep = new CBaseTestSDBigFileRead();
		}
	else if (aStepName == KTestStepBigFileWrite)
		{
		testStep = new CBaseTestSDBigFileWrite();
		}
	else if (aStepName == KTestStepCheckDisk)
		{
		testStep = new CBaseTestSDCheckDisk();
		}
	else if (aStepName == KTestStepCopyFile)
		{
		testStep = new CBaseTestSDCopyFile();
		}
	else if (aStepName == KTestStepDialogBox)
		{
		testStep = new CBaseTestSDDialogBox();
		}
	else if (aStepName == KTestStepDisplayInfo)
		{
		testStep = new CBaseTestSDDisplayInfo();
		}
	else if (aStepName == KTestStepFieldCheck)
		{
		testStep = new CBaseTestSDFieldCheck(*this);
		}
	else if (aStepName == KTestStepFileOperations1)
		{
		testStep = new CBaseTestSDFileOperations1();
		}
	else if (aStepName == KTestStepFileOperations2)
		{
		testStep = new CBaseTestSDFileOperations2();
		}
	else if (aStepName == KTestStepFillSectors)
		{
		testStep = new CBaseTestSDFillSectors();
		}
	else if (aStepName == KTestStepFormat)
		{
		testStep = new CBaseTestSDFormat();
		}
	else if (aStepName == KTestStepReadFiles1)
		{
		testStep = new CBaseTestSDReadFiles1();
		}
	else if (aStepName == KTestStepReadFiles2)
		{
		testStep = new CBaseTestSDReadFiles2();
		}
	else if (aStepName == KTestStepUsb)
		{
		testStep = new CBaseTestSDUsb();
		}
	return testStep;
	}
