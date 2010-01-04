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




#ifndef  __T_PERF_UTILS_CLEANUP_STEP_H__
#define  __T_PERF_UTILS_CLEANUP_STEP_H__


//user includes
#include "UtilsStep.h"

/*@{*/
// Literals Used 
_LIT(KT_CleanupStep,	"CleanupStep");
/*@}*/


class  CT_CleanupStep  : public CT_UtilsStep
/**
@test
@publishedPartner
*/
	{	
public:
	~CT_CleanupStep();
	CT_CleanupStep();
	TVerdict	doTestStepL();
	};


#endif /*  __T_PERF_UTILS_CLEANUP_STEP_H__ */
