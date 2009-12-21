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




#ifndef __BASE_STEP_H__
#define __BASE_STEP_H__

// User includes
#include "TestStepV2.h"

//Epoc includes
#include <f32file.h>
#include <e32cmn.h>
#include <e32std.h>

class CT_F32BaseStep : public CTestStepV2
/**
@test
@publishedPartner
*/
	{
public:
	~CT_F32BaseStep();
	CT_F32BaseStep(TBool aOpenFiles);
	TVerdict	doTestStepL();
	TVerdict	doTestStepPreambleL();
	TVerdict    doTestStepPostambleL();

protected:
	virtual TInt 		ThreadFuncL(RFs& aSession)=0;
	virtual void 		ThreadPostFuncL(RFs& aSession);
	virtual TInt 		ThreadPreFuncL(RFs& aSession);
	virtual void 		PrintResults();

private:
	inline static TInt 	ThreadFunction(TAny* aPtr);

	void				SetUpL();
	TBool				SetOwnerType(const TDesC& aName, TOwnerType& aOwnerType);
	TBool				SetThreadPriority(const TDesC& aName, TThreadPriority& aThreadPriority);
	void				SetFilePathArrayL();
	void				CreateCsvFileL();
	TInt				AppendDataL(RFile& aFile, TDesC& aString);

protected:
	RArray<RFile>				iFileArray;
	RArray<TFileName>			iFilePathArray;
	TInt						iFuncCalls;
	TInt						iNumOfFiles;
	TInt						iDirTreeDepth;
	TInt						iFileSize;
	TPtrC						iDirBaseName;
	TPtrC						iFileBaseName;
	TPtrC						iDirSubName;
	TInt						iBlockSize;
	TTimeIntervalMicroSeconds	iTotalTime;

private:
	TPtrC						iCsvFileName;
	TPtrC						iPtr;
	TPtrC						iOperation;
	TBool						iOpenFiles;
};

#include "BaseStep.inl"
#endif /* __BASE_STEP_H__ */
