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


/**
@test
@internalComponent

This contains CT_ActiveNotifyChange
*/

//	User includes
#include "T_ActiveNotifyChange.h"

CT_ActiveNotifyChange* CT_ActiveNotifyChange::NewL(TInt aCount, TInt aAsyncErrorIndex, MActiveCallback& aCallback, TInt aPriority)
/**
 * Two phase constructor
 */
	{
	CT_ActiveNotifyChange*	ret = new (ELeave) CT_ActiveNotifyChange(aCount, aAsyncErrorIndex, aCallback, aPriority);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;	
	}

CT_ActiveNotifyChange* CT_ActiveNotifyChange::NewLC(TInt aCount, TInt aAsyncErrorIndex, MActiveCallback& aCallback, TInt aPriority)
/**
 * Two phase constructor
 */
	{
	CT_ActiveNotifyChange*	ret = new (ELeave) CT_ActiveNotifyChange(aCount, aAsyncErrorIndex, aCallback, aPriority);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	return ret;	
	}

CT_ActiveNotifyChange::CT_ActiveNotifyChange(TInt aCount, TInt aAsyncErrorIndex, MActiveCallback& aCallback, TInt aPriority)
/**
 * Protected constructor. First phase construction
 */
:	CActiveCallback(aCallback, aPriority)
,	iCount(aCount)
,	iAsyncErrorIndex(aAsyncErrorIndex)
	{
	}

void CT_ActiveNotifyChange::Activate()
	{
	CActiveCallback::Activate(iAsyncErrorIndex);
	}

TInt CT_ActiveNotifyChange::DecCount()
	{
	return --iCount;
	}
