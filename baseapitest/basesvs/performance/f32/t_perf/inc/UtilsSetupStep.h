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




#ifndef  __T_PERF_UTILS_SETUP_STEP_H__
#define __T_PERF_UTILS_SETUP_STEP_H__



//user includes
#include "UtilsStep.h"

/*@{*/
//Literals used
_LIT(KT_SetupStep,	"SetupStep" );
/*@}*/


class  CT_SetupStep  : public CT_UtilsStep
/**
@test
@publishedPartner
*/
	{
public:
	~CT_SetupStep();
	CT_SetupStep();
	TVerdict	doTestStepL();
	TVerdict	doTestStepPreambleL();

private:
	void	CreateDirTreeL();
	void	CreateFileL(TDesC& aPath);
	void	CreateFileDataL(RFile& aFile);


private:
	TInt	iFileSize;
	TInt	iDirTreeDepth;
	TInt	iNumOfFiles;
	TPtrC	iFileBaseName;
	TPtrC	iDirSubName;
	TPtrC	iFileData;

	};


#endif /* __T_PERF_UTILS_SETUP_STEP_H__ */
