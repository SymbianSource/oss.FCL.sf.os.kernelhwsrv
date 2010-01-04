/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/




#ifndef __F32_PERFORMANCE_SERVER_H__
#define __F32_PERFORMANCE_SERVER_H__


//	Epoc includes
#include <test/testexecuteserverbase.h>


/*@{*/
//Literals used
_LIT(KT_PerformServ,	"t_perf");
/*@}*/

// Implements the  which creates the  test steps 
class CT_F32PerformanceServer : public CTestServer 
/**
@test
@publishedPartner
*/
	{
public:
	// Creates an object of type CT_F32PerformanceServer 
	static CT_F32PerformanceServer*  NewL();
	~CT_F32PerformanceServer();
	
protected:

	CT_F32PerformanceServer();

	//
	// Creates the test steps based on the test step name 
	// read from the script file
	//
	virtual CTestStep*	CreateTestStep(const TDesC& aStepName);

	};



#endif /* __F32_PERFORMANCE_SERVER_H__ */

