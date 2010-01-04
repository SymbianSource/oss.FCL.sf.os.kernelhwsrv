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

This contains CT_FileActiveCallback
*/

#if (!defined __T_FILEACTIVECALLBACK_H__)
#define __T_FILEACTIVECALLBACK_H__

#include <test/activecallback.h>

class CT_FileActiveCallback : public CActiveCallback
	{
public:
	static CT_FileActiveCallback*	NewL(MActiveCallback& aCallback, TInt aPriority=EPriorityStandard);
	static CT_FileActiveCallback*	NewLC(MActiveCallback& aCallback, TInt aPriority=EPriorityStandard);

	TInt	DecCount();
	void	Activate();
	void 	Activate(TInt aAsuncErrorIndex);
	
	void	SetSection(const TDesC& aSection);
	
	void	CreateFileDataBufferL(TInt aLength);
	
	TInt	iType;
	HBufC8* 	iFileData;
	TDesC*  iSection;

protected:
	CT_FileActiveCallback(MActiveCallback& aCallback, TInt aPriority);
	~CT_FileActiveCallback();

private:
	TInt	iCount;
	TInt	iAsyncErrorIndex;
	};

#endif /* __T_FILEACTIVECALLBACK_H__ */
