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




#ifndef __SEEK_FILE_STEP_H__
#define __SEEK_FILE_STEP_H__




//User includes
#include "BaseStep.h"

/*@{*/
//Literals used
_LIT(KT_SeekFileStep,	"SeekFileStep");
/*@}*/


class CT_SeekFileStep:public CT_F32BaseStep
/**
@test
@publishedPartner
*/
	{
public:
	CT_SeekFileStep();
	~CT_SeekFileStep();
	TVerdict	doTestStepPreambleL();

protected:
	virtual TInt	ThreadFuncL(RFs& aSession);

private:
	void			SetSeekMode(TDesC& aSeekMode);

protected:
	TSeek	iSeekMode;
	};


#endif /* __SEEK_FILE_STEP_H__ */
