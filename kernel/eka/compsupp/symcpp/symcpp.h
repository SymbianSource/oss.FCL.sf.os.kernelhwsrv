// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// This is a preinclude file for the Symbian C++ specific defintions for RVCT.
// 
//

// Deal with operator new issues here
#ifdef __cplusplus
namespace std {
	struct nothrow_t { };
	extern const nothrow_t nothrow;
}

#ifndef __OPERATOR_NEW_DECLARED__
#define __OPERATOR_NEW_DECLARED__

/* e32cmn.h also declares these five overloads, but slightly differently, so use
 * __OPERATOR_NEW_DECLARED__ to avoid the declarations here (included by compiler-specific
 * pre-include files) from conflicting.
 */

IMPORT_C void* operator new(unsigned int aSize) __NO_THROW;

IMPORT_C void* operator new(unsigned int aSize,unsigned int aSize1) __NO_THROW;

IMPORT_C void* operator new[](unsigned int aSize) __NO_THROW;

IMPORT_C void operator delete(void* aPtr) __NO_THROW;

IMPORT_C void operator delete[](void* aPtr) __NO_THROW;

#endif // !__OPERATOR_NEW_DECLARED__


/* The following four overloads are not declared by the generic Symbian headers, so
 * do not need to be protected by __OPERATOR_NEW_DECLARED__.
 */

IMPORT_C void* operator new(unsigned int aSize, const std::nothrow_t& aNoThrow) __NO_THROW;

IMPORT_C void* operator new[](unsigned int aSize, const std::nothrow_t& aNoThrow) __NO_THROW;

IMPORT_C void operator delete(void* aPtr, const std::nothrow_t& aNoThrow) __NO_THROW;

IMPORT_C void operator delete[](void* aPtr, const std::nothrow_t& aNoThrow) __NO_THROW;

#endif
