// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\euser\us_hand.cpp
// 
//

#include "us_std.h"


_LIT(KLiteralMatchAny, "*");

EXPORT_C TFindHandleBase::TFindHandleBase()
	: iMatch(KLiteralMatchAny)
/**
Default constructor.

The default constructed TFindHandleBase object has the default match pattern, i.e. 
the single character "*".
*/
	{
	}




EXPORT_C TFindHandleBase::TFindHandleBase(const TDesC& aMatch)
	: iMatch(aMatch)
/**
Constructor with match pattern.

This constructor creates a TFindHandleBase object with the specified match 
pattern.

@param aMatch A reference to the descriptor containing the match pattern.
*/
	{
	}




EXPORT_C void TFindHandleBase::Find(const TDesC& aMatch)
/**
Sets a new match pattern.

On return from this function, this TFindHandleBase object contains a copy 
of the supplied match pattern; the source descriptor can, therefore, be safely 
discarded.

@param aMatch A reference to the descriptor containing the new match pattern.
*/
	{

	Reset();
	iMatch=aMatch;
	}

/**
	Implementation for TFindXxxxxxx::Next(TFullName &aResult) methods
	@internalComponent
*/
TInt TFindHandleBase::NextObject(TFullName& aResult, TInt aObjectType)
	{
	TBuf8<KMaxFullName> match8;
	match8.Copy(iMatch);
	TInt r = Exec::ObjectNext((TObjectType)aObjectType, match8, *this);
	if (r==KErrNone)
		{
		aResult.Copy(match8);
		}
	return r;
	}

EXPORT_C void RHandleBase::SetHandleNC(TInt aHandle)
/**
Sets the handle-number of this handle to the specified 
value, and marks it as not closable.

@param aHandle The handle-number to be set.
*/
	{
	SetHandle(aHandle|CObjectIx::ENoClose);
	}
