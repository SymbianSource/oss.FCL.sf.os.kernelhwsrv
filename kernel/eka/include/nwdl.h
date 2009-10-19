// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nwdl.h
// 
//

#ifndef __NWDL_H__
#define __NWDL_H__

/**
@file
@internalTechnology
*/

#ifndef __NO_ALLOC_DECLARATIONS__
#ifdef __KERNEL_MODE__
#define __ALLOCATOR Kern
#else
#define __ALLOCATOR User
#endif

GLDEF_C TAny *operator new(TUint aSize) __NO_THROW
//
// The global new operator.
//
	{
#ifdef __USE_MALLOC__
	return malloc(aSize);
#else
	return __ALLOCATOR::Alloc(aSize);
#endif
	}

GLDEF_C TAny *operator new[](TUint aSize) __NO_THROW
    {
#ifdef __USE_MALLOC__
	return malloc(aSize);
#else
	return __ALLOCATOR::Alloc(aSize);
#endif
    }

GLDEF_C TAny *operator new(TUint aSize, TUint anExtraSize) __NO_THROW
//
// Allocate the requested size plus the extra.
//
	{
#ifdef __USE_MALLOC__
	return malloc(aSize+anExtraSize);
#else
	return __ALLOCATOR::Alloc(aSize+anExtraSize);
#endif
	}

GLDEF_C void operator delete(TAny *aPtr) __NO_THROW
//
// The replacement delete operator.
//
	{
#ifdef __USE_MALLOC__
	free(aPtr);
#else
	__ALLOCATOR::Free(aPtr);
#endif
	}

GLDEF_C void operator delete[](TAny * aPtr) __NO_THROW
    {
#ifdef __USE_MALLOC__
	free(aPtr);
#else
	__ALLOCATOR::Free(aPtr);
#endif
    }
#endif
#endif // #ifndef __NWDL_H__
