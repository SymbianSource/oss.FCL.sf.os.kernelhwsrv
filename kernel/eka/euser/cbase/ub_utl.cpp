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
// e32\euser\cbase\ub_utl.cpp
// 
//

#include "ub_std.h"

_LIT(KE32UserCBase, "E32USER-CBase");
GLDEF_C void Panic(TBasePanic aPanic)
//
// Panic the process with E32USER-ADT as the category.
//
	{

	User::Panic(KE32UserCBase, aPanic);
	}




/**
Virtual destructor.

Enables any derived object to be deleted through a CBase* pointer.
*/
EXPORT_C CBase::~CBase()
	{
	}



/**
Extension function


*/
EXPORT_C TInt CBase::Extension_(TUint, TAny*& a0, TAny*)
	{
	a0 = NULL;
	return KErrExtensionNotSupported;
	}




/**
Deletes the specified object.

@param aPtr Pointer to the CBase derived object to be deleted.
*/
EXPORT_C void CBase::Delete(CBase* aPtr)
	{
	delete aPtr;
	}

