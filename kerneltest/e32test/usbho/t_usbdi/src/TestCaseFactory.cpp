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
// @file testcasefactory.cpp
// @internalComponent
// 
//

#include "TestCaseFactory.h"
#include "testdevicebase.h"
#include <e32debug.h>

namespace NUnitTesting_USBDI
	{
	
RTestFactory& RTestFactory::Instance()
	{
	static RTestFactory singleton;
	return singleton;
	}
	
RTestFactory::~RTestFactory()
	{
	}
	
	
RTestFactory::RTestFactory()
:	iTestCases(TStringIdentity::Hash,TStringIdentity::Id)
	{
	}
	
void RTestFactory::RegisterTestCase(const TDesC& aTestCaseId,TBaseTestCaseFunctor const* aFunctor)
	{
	LOG_CFUNC

	LOG_INFO((_L("Registering test case '%S'"),&aTestCaseId))
	
	TStringIdentity key(aTestCaseId);
	TInt err(Instance().iTestCases.Insert(key,aFunctor));
	if(err != KErrNone)
		{
		// Log that a test case could not be registered due to err
		RDebug::Printf("Test case '%S' could not be registered with test case factory",&aTestCaseId);
		}
	}


CBaseTestCase* RTestFactory::CreateTestCaseL(const TDesC& aTestCaseId,TBool aHostRole)
	{
	LOG_CFUNC
	
	TStringIdentity key(aTestCaseId);
	const TBaseTestCaseFunctor& functor = *(*Instance().iTestCases.Find(key));
	return functor(aHostRole);
	}
	

void RTestFactory::ListRegisteredTestCases()
	{
	LOG_CFUNC
	RFactoryMap::TIter it(Instance().iTestCases);
	
	RDebug::Printf("-------- F A C T O R Y ---------");
	
	TInt count(0);
	for(count=0; count<Instance().iTestCases.Count(); count++)
		{
		RDebug::Printf("%d: %S",count,it.NextKey());
		}
	
	RDebug::Printf("--------------------------------");
	}

	}
