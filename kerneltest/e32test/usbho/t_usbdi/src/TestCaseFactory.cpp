// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file testcasefactory.cpp
// @internalComponent
// 
//

#include "TestCaseFactory.h"
#include "testdevicebase.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "TestCaseFactoryTraces.h"
#endif
#include <e32debug.h>

namespace NUnitTesting_USBDI
	{
	
RTestFactory& RTestFactory::Instance()
	{
	OstTraceFunctionEntry1( RTESTFACTORY_INSTANCE_ENTRY, 0 );
	static RTestFactory singleton;
	OstTraceFunctionExitExt( RTESTFACTORY_INSTANCE_EXIT, 0, ( TUint )&( singleton ) );
	return singleton;
	}
	
RTestFactory::~RTestFactory()
	{
	OstTraceFunctionEntry1( RTESTFACTORY_RTESTFACTORY_ENTRY, this );
	OstTraceFunctionExit1( RTESTFACTORY_RTESTFACTORY_EXIT, this );
	}
	
	
RTestFactory::RTestFactory()
:	iTestCases(TStringIdentity::Hash,TStringIdentity::Id)
	{
	OstTraceFunctionEntry1( RTESTFACTORY_RTESTFACTORY_ENTRY_DUP01, this );
	OstTraceFunctionExit1( RTESTFACTORY_RTESTFACTORY_EXIT_DUP01, this );
	}
	
void RTestFactory::RegisterTestCase(const TDesC& aTestCaseId,TBaseTestCaseFunctor const* aFunctor)
	{
    OstTraceFunctionEntryExt( RTESTFACTORY_REGISTERTESTCASE_ENTRY, 0 );

	LOG_INFO((_L("Registering test case '%S'"),&aTestCaseId))
	
	TStringIdentity key(aTestCaseId);
	TInt err(Instance().iTestCases.Insert(key,aFunctor));
	if(err != KErrNone)
		{
		// Log that a test case could not be registered due to err
		OstTraceExt1(TRACE_NORMAL, RTESTFACTORY_REGISTERTESTCASE, "Test case '%S' could not be registered with test case factory",aTestCaseId);
		}
	OstTraceFunctionExit1( RTESTFACTORY_REGISTERTESTCASE_EXIT, 0 );
	}


CBaseTestCase* RTestFactory::CreateTestCaseL(const TDesC& aTestCaseId,TBool aHostRole)
	{
	OstTraceFunctionEntryExt( RTESTFACTORY_CREATETESTCASEL_ENTRY, 0 );
	
	TStringIdentity key(aTestCaseId);
	const TBaseTestCaseFunctor& functor = *(*Instance().iTestCases.Find(key));
	return functor(aHostRole);
	}
	

void RTestFactory::ListRegisteredTestCases()
	{
	OstTraceFunctionEntry1( RTESTFACTORY_LISTREGISTEREDTESTCASES_ENTRY, 0 );
	RFactoryMap::TIter it(Instance().iTestCases);
	
	OstTrace0(TRACE_NORMAL, RTESTFACTORY_LISTREGISTEREDTESTCASES, "-------- F A C T O R Y ---------");
	
	TInt count(0);
	for(count=0; count<Instance().iTestCases.Count(); count++)
		{
		OstTrace1(TRACE_NORMAL, RTESTFACTORY_LISTREGISTEREDTESTCASES_DUP01, "%d",count);
		}
	
	OstTrace0(TRACE_NORMAL, RTESTFACTORY_LISTREGISTEREDTESTCASES_DUP02, "--------------------------------");
	OstTraceFunctionExit1( RTESTFACTORY_LISTREGISTEREDTESTCASES_EXIT, 0 );
	}

	}
