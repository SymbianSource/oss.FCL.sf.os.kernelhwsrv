#ifndef __TEST_CASE_CONTROLLER_H
#define __TEST_CASE_CONTROLLER_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file TestCaseController.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <e32test.h>
#include "TestCaseFactory.h"

namespace NUnitTesting_USBDI
	{
	
// Forward declarations

class CTestEngine;
class CBasicTestPolicy;
class CBaseTestCase;

/**
This class represents the test case state machine for executing the host test cases
*/
class CTestCaseController : public CActive
	{
public:
	/**
	Symbian construction of the controller of the test cases
	@param aTestEngine the test engine for the test application
	@param aHostRole the role for the test controller (i.e. running test cases in host or client mode)
	*/
	
	static CTestCaseController* NewL(CTestEngine& aTestEngine,TBool aHostRole);
	
	/**
	Destructor
	*/
	
	~CTestCaseController();
	
protected: // From CActive
	
	/**
	Cancels the task that the this controller is performing
	*/
	
	void DoCancel();
	
	/**
	Keeps running all specified test cases
	*/
	
	void RunL();
	
	/**
	Framework error function for RunL
	*/
	
	TInt RunError(TInt aError);
	
private:
	
	/**
	C++ constructor, build a controller for the test cases
	@param aTestEngine the test engine for the test application
	@param aHostRole the role for the test controller (i.e. running test cases in host or client mode)
	*/

	CTestCaseController(CTestEngine& aTestEngine,TBool aHostRole);
	
	/**
	Symbian 2nd phase construction
	*/
	
	void ConstructL();
	
private:
	/**
	The test engine
	*/
	CTestEngine& iTestEngine;
	
	/**
	*/
	TBuf<KTestCaseIdLength> iTestCaseId;
	
	/**
	Array of cases results
	*/
	RArray<TBool> iTestCasesResults;
	
	/**
	The test policy employed for the test cases
	*/
	CBasicTestPolicy* iTestPolicy;
	
	/**
	The role of this controller, whether it runs USB client device test cases
	or USB Host test cases
	*/
	TBool iHostRole;
	};

	}

#endif