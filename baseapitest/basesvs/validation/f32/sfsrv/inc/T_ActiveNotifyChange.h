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

#if (!defined __T_ACRIVE_NOTIFY_CHANGE_H__)
#define __T_ACRIVE_NOTIFY_CHANGE_H__

#include <test/activecallback.h>

class CT_ActiveNotifyChange : public CActiveCallback
	{
public:
	static CT_ActiveNotifyChange*	NewL(TInt aCount, TInt aAsyncErrorIndex, MActiveCallback& aCallback, TInt aPriority=EPriorityStandard);
	static CT_ActiveNotifyChange*	NewLC(TInt aCount, TInt aAsyncErrorIndex, MActiveCallback& aCallback, TInt aPriority=EPriorityStandard);

	TInt	DecCount();
	void	Activate();

protected:
	CT_ActiveNotifyChange(TInt aCount, TInt aAsyncErrorIndex, MActiveCallback& aCallback, TInt aPriority);

private:
	TInt	iCount;
	TInt	iAsyncErrorIndex;
	};

#endif /* __T_ACRIVE_NOTIFY_CHANGE_H__ */
