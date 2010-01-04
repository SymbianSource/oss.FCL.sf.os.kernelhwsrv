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





#ifndef __ENTRY_STEP_H__
#define __ENTRY_STEP_H__

//User includes
#include "BaseStep.h"

/*@{*/
//Literals used
_LIT(KT_EntryStep,	"EntryStep");
/*@}*/

class CT_EntryStep: public CT_F32BaseStep
/**
@test
@publishedPartner
*/
	{
public:
	CT_EntryStep();
	~CT_EntryStep();

protected:
 	TVerdict 		doTestStepPreambleL();
	virtual TInt	ThreadFuncL(RFs& aSession);
	};

#endif /* __ENTRY_STEP_H__ */
