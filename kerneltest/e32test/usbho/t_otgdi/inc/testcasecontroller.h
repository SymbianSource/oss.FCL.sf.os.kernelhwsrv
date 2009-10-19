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
// @internalComponent
// 
//

#ifndef __TEST_CASE_CONTROLLER_H
#define __TEST_CASE_CONTROLLER_H

#include "TestCaseFactory.h"

	
// Forward declarations

class CTestEngine;
class CBasicTestPolicy;

/**
This class represents the test case state machine for executing the OTG test cases
*/
class CTestCaseController : public CActive
	{
public:
	static CTestCaseController* NewL(CTestEngine& aTestEngine,TBool aHostRole);
	~CTestCaseController();
	
	
protected: // From CActive
	virtual void DoCancel();
	virtual void RunL();
	virtual TInt RunError(TInt aError);
	
private:
	CTestCaseController(CTestEngine& aTestEngine,TBool aHostRole);
	void ConstructL();
	
private:
	CTestEngine& iTestEngine;
	TBuf<KTestCaseIdLength> iTestCaseId;
	CBasicTestPolicy* iTestPolicy;
	TBool iHostRole;
	};


#endif // __TEST_CASE_CONTROLLER_H
