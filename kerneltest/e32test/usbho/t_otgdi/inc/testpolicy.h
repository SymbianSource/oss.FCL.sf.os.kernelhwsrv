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
* @file TestPolicy.h
* @internalComponent
* 
*
*/



#ifndef TESTPOLICY_H
#define TESTPOLICY_H

#include <e32base.h>
#include <e32test.h>



/** In OTGDI suite, this does not do any policy/decision making, it merely instantiates the test
*/
class CBasicTestPolicy : public CActive
	{
public:
	static CBasicTestPolicy* NewL();
	virtual ~CBasicTestPolicy();
	
	virtual void RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus* aStatus);
	void SignalTestComplete(TInt aCompletionCode);

private:
	CBasicTestPolicy();
	/**
	2nd phase constructor
	*/
	void ConstructL();	
	
protected:
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);

	// DATA
public: 
	CTestCaseRoot* iTestCase;

private:
	TRequestStatus* iNotifierStatus; 
	};
	

#endif // TESTPOLICY_H
