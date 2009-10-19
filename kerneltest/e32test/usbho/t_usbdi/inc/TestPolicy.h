#ifndef __TEST_POLICY_H
#define __TEST_POLICY_H

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



#include <e32base.h>
#include <d32usbdi_hubdriver.h>
#include <e32test.h>

extern RTest gtest;

namespace NUnitTesting_USBDI
	{

// Forward declarations

class CBaseTestCase;

/**
*/
class CBasicTestPolicy : public CActive
	{
public:
	static CBasicTestPolicy* NewL();
	
	/**
	Destructor
	*/
	
	virtual ~CBasicTestPolicy();
	
	/**
	Run the test case with the specified identity and receive notification of test case
	success or failiure completion
	@param aTestCaseId the identity of the test case to run
	@param aStatus the request status of the entity that wants test case completion notification
	*/
	
	virtual void RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus& aStatus);
	
	/**
	*/
	
	virtual void SignalTestComplete(TInt aCompletionCode);

protected:
	CBasicTestPolicy();
	void ConstructL();

protected:
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);

protected: 
	CBaseTestCase* iTestCase;
	TRequestStatus* iNotifierStatus; 
	};
	
/**
*/
class CThreadTestPolicy : public CBasicTestPolicy
	{
public:
	static CThreadTestPolicy* NewL();
	
	/**
	Destructor
	*/
	
	virtual ~CThreadTestPolicy();
	
	/**
	Run the test case with the specified identity and receive notification of test case
	success or failiure completion
	@param aTestCaseId the identity of the test case to run
	@param aStatus the request status of the entity that wants test case completion notification
	*/
	
	virtual void RunTestCaseL(const TDesC& aTestCaseId,TRequestStatus& aStatus);

	/**
	*/

	virtual void SignalTestComplete(TInt aCompletionCode);
	
protected:
	void DoCancel();
	void RunL();
	TInt RunError(TInt aError);

private:
	CThreadTestPolicy();

	void ConstructL();
	
	static TInt ThreadFunction(TAny* aThreadParameter);
	static TInt DoTestL(const TDesC& aTestCaseId);
	
private:
	/**
	Each test case executes its test code in a spawned thread, so iTestThread is 
	the handle to the spawned test thread
	*/
	RThread iTestThread;
	
	HBufC* iTestCaseId;
	};


	}

#endif
