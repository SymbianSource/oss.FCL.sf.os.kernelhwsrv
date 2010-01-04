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





#ifndef __BASE_STEP_INL__
#define __BASE_STEP_INL__

TInt CT_F32BaseStep::ThreadFunction(TAny* aPtr)
	{
	CT_F32BaseStep*	mythread=static_cast<CT_F32BaseStep*>(aPtr);
	CTrapCleanup*	cleanup=CTrapCleanup::New();
	RFs				session;

	TRAPD(result, mythread->ThreadPreFuncL(session));
	if(result==KErrNone )
		{
		TRAPD(result, mythread->ThreadFuncL(session));
		mythread->ThreadPostFuncL(session);
		}
		
	delete cleanup;
	cleanup=NULL;
	return result;
	}



#endif /* __BASE_STEP_INL__ */


