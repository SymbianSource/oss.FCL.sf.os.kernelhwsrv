#ifndef __TEST_ENGINE_H
#define __TEST_ENGINE_H

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
* @file TestEngine.h
* @internalComponent
* 
*
*/



#include <e32base.h>

namespace NUnitTesting_USBDI
	{ 
	
// Forward declarations

class CTestCaseController;
	
/**
This class represents the engine for this test module.
It parses the command line and builds the test that should be run(through iTestCaseController)
*/
class CTestEngine : public CActive
	{
public:
	/**
	Symbian Construction
	@return a pointer to a new instance of CTestEngine
	*/
	
	static CTestEngine* NewL();

	/**
	Destructor
	*/
	
	~CTestEngine();
	
	/**
	Retrieve the next in the list of test cases identities to run
	@param an empty descriptor to populate with the next test case identity
	@return KErrNone if successful or KErrNotFound if test cases identities depleted
	*/
	
	TInt NextTestCaseId(TDes& aTestCaseId);

	/**
	Retrieve the list of test cases identities
	@return the list of test cases identities
	*/
	
	RPointerArray<HBufC>& TestCasesIdentities();
	
	/**
	Retrieve the number of times the test cases are repeated
	@the number of times the test cases are repeated
	*/
	
	TUint NumRepeats();
	
private: // From CActive
	/**
	*/
	
	void DoCancel();
	
	/**
	*/
	
	void RunL();
	
	/**
	*/
	
	TInt RunError(TInt aError);	
	
private:
	/**
	C++ constructor
	*/
	
	CTestEngine();
	
	/**
	2nd phase constructor
	*/
	
	void ConstructL();

private:
	/**
	The test cases from the Xml file that the user wishes to execute
	*/
	RPointerArray<HBufC> iTestCasesIdentities;
	
	/**
	*/
	TInt iTestCaseIndex;
		
	/**
	*/
	TUint iRepeat;
				
	/**
	*/
	TUint iNumRepeats;

	/**
	The test controller
	*/
	CTestCaseController* iTestCaseController;
	};

	}
	
#endif