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





#ifndef __READ_FILE_STEP_H__
#define __READ_FILE_STEP_H__

//User includes
#include "SeekFileStep.h"


/*@{*/
//Literals used
_LIT(KT_ReadFileStep,	"ReadFileStep");
/*@}*/

class CT_ReadFileStep:  public  CT_SeekFileStep
/**
@test
@publishedPartner
*/
	{
public:
	CT_ReadFileStep();
	~CT_ReadFileStep();
	TVerdict		doTestStepPreambleL();

protected:
	virtual TInt	ThreadFuncL(RFs& aSession);

protected:
	TBool	iSeek;
	};

#endif /* __T_PERF_READ_FILE_STEP_H__ */
