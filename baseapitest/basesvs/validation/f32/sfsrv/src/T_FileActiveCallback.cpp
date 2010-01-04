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

//	User includes
#include "T_FileActiveCallback.h"

CT_FileActiveCallback* CT_FileActiveCallback::NewL(MActiveCallback& aCallback, TInt aPriority)
/**
 * Two phase constructor
 */
	{
	CT_FileActiveCallback*	ret = NewLC(aCallback, aPriority);
	CleanupStack::Pop(ret);
	return ret;	
	}

CT_FileActiveCallback* CT_FileActiveCallback::NewLC(MActiveCallback& aCallback, TInt aPriority)
/**
 * Two phase constructor
 */
	{
	CT_FileActiveCallback*	ret = new (ELeave) CT_FileActiveCallback(aCallback, aPriority);
	CleanupStack::PushL(ret);
	ret->ConstructL();
	return ret;	
	}

CT_FileActiveCallback::CT_FileActiveCallback(MActiveCallback& aCallback, TInt aPriority)
/**
 * Protected constructor. First phase construction
 */
:	CActiveCallback(aCallback, aPriority),
    iFileData(NULL),
    iSection(NULL)
	{
	}
	
CT_FileActiveCallback::~CT_FileActiveCallback()
	{ 	
	delete iFileData;
	iFileData = NULL;
	}

void CT_FileActiveCallback::Activate()
	{
	CActiveCallback::Activate(iAsyncErrorIndex);
	}
	
void CT_FileActiveCallback::Activate(TInt aAsyncErrorIndex)
	{
	CActiveCallback::Activate(aAsyncErrorIndex);
	}	

TInt CT_FileActiveCallback::DecCount()
	{
	return --iCount;
	}

void CT_FileActiveCallback::SetSection(const TDesC& aSection)
	{
	this->iSection=const_cast<TDesC*>(&aSection);
	}
	
void CT_FileActiveCallback::CreateFileDataBufferL(TInt aLength)
	{
	iFileData = HBufC8::NewL(aLength);
	}

