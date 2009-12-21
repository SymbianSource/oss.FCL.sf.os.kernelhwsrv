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




#ifndef  __T_PERF_UTILS_STEP_H__
#define __T_PERF_UTILS_STEP_H__


//Epoc includes
#include <f32file.h>
#include <e32cmn.h>
#include<e32std.h>

// User includes
#include "TestStepV2.h"


class  CT_UtilsStep  : public CTestStepV2
/**
@test
@publishedPartner
*/
	{
public:
	~CT_UtilsStep();
	CT_UtilsStep();
	TVerdict	doTestStepPreambleL();

protected:
	void	DoFormatDriveL();
	TInt	RemoveDirTreeL();

protected:
	TBool	iFormatDrive;
	TPtrC	iDirBaseName;
	RFs		iFsSession;
	};


#endif /* __T_PERF_UTILS_STEP_H__ */
