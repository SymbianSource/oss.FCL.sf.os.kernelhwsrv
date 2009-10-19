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

#ifndef TESTCASEWD_H
#define TESTCASEWD_H

	
// method prototype used to pass a 'this' pointer to a friend
typedef void (*WDCancellerMethod)(CTestCaseRoot* pThis);

class CTestCaseRoot;

class CTestCaseWatchdog : public CTimer
	{
public:	
	~CTestCaseWatchdog();

	  // Static construction
	static CTestCaseWatchdog* NewL();

	  // issue request
	void IssueRequest(TInt aWatchdogIntervalMS, CTestCaseRoot* pRoot, WDCancellerMethod cancelMethod); //ms duration 

	  // service completed request.
	  // Defined as pure virtual by CActive;
	  // implementation provided by this class.
	void RunL();

private:
	CTestCaseWatchdog();
	  // Second phase construction
	void ConstructL(); 
	
	TBool IsValid();

	// DATA
protected:
	TInt iMSRequested;  // Total MS to watchdog for
	CTestCaseRoot	*iThisPointer;
	WDCancellerMethod iCancelFriendFunc;
	
	TInt	iValidConstr;
	};
	        
	

#endif // TESTCASEWD_H
