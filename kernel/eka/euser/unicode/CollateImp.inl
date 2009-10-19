// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 
 @param aMethod Defines the collation method to be used in the iterations.
 @internalComponent
*/
inline TCollationValueIterator::TCollationValueIterator(const TCollationMethod& aMethod) : 
    iMethod(aMethod) 
    {
    }

/**
@return ETrue The method forbids ignoring characters, EFalse otherwise.
@internalComponent
*/
inline TBool TCollationValueIterator::IgnoringNone() const 
    { 
    return iMethod.iFlags & TCollationMethod::EIgnoreNone; 
    }

/**
@return A const reference to the used collation method.
@internalComponent
*/
inline const TCollationMethod& TCollationValueIterator::CollationMethod() const 
    { 
    return iMethod; 
    }

