/*
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Map ANSI bsearch and qsort onto EPOC32 functions 
*/

#include <e32std.h>
#include "openc.h"

NONSHARABLE_CLASS(TAnsiKey) : public TKey
	{
public:
	TAnsiKey(const TAny* key, const TAny* base, TInt length, TInt (*compar)(const TAny*, const TAny*))
		: iSearchKey(key), iCmp(compar)
		{ SetPtr(base); iKeyLength=length; }

	virtual TInt Compare(TInt aLeft,TInt aRight) const;
	virtual TAny *At(TInt anIndex) const;
private:
	const TAny* iSearchKey;
	TInt (*iCmp)(const TAny*, const TAny*);
	};

TInt TAnsiKey::Compare (TInt aLeft,TInt aRight) const
	{
	if (aRight==KIndexPtr)
		return (*iCmp)(At(aLeft),iSearchKey);
	else
		return (*iCmp)(At(aLeft),At(aRight));
	}

TAny* TAnsiKey::At (TInt aPos) const
	{
	return (TUint8*)iPtr+(aPos*iKeyLength);
	}

NONSHARABLE_CLASS(TAnsiSwap) : public TSwap
	{
public:
	TAnsiSwap(const TAny* base, TInt length) : iPtr(base), iLength(length) {}

	virtual void Swap(TInt aLeft,TInt aRight) const;
private:
	TUint8* At(TInt aPos) const {return (TUint8*)iPtr+(aPos*iLength);}

	const TAny* iPtr;
	TInt  iLength;
	};

void TAnsiSwap::Swap(TInt aLeft,TInt aRight) const
	{
	TUint8* left=At(aLeft);
	TUint8* right=At(aRight);
	TUint8 tmp;

	for (TInt i=iLength; i>0; i--)
		{
		tmp=*left;
		*left++=*right;
		*right++=tmp;
		}
	}

/*
FUNCTION
<<bsearch>>---binary search

INDEX
	bsearch

ANSI_SYNOPSIS
	#include <stdlib.h>
	void *bsearch(const void *<[key]>, const void *<[base]>,
		size_t <[nmemb]>, size_t <[size]>,
		int (*<[compar]>)(const void *, const void *));

TRAD_SYNOPSIS
	#include <stdlib.h>
	char *bsearch(<[key]>, <[base]>, <[nmemb]>, <[size]>, <[compar]>)
	char *<[key]>;
	char *<[base]>;
	size_t <[nmemb]>, <[size]>;
	int (*<[compar]>)();

DESCRIPTION
<<bsearch>> searches an array beginning at <[base]> for any element
that matches <[key]>, using binary search.  <[nmemb]> is the element
count of the array; <[size]> is the size of each element.

The array must be sorted in ascending order with respect to the
comparison function <[compar]> (which you supply as the last argument of
<<bsearch>>).

You must define the comparison function <<(*<[compar]>)>> to have two
arguments; its result must be negative if the first argument is
less than the second, zero if the two arguments match, and
positive if the first argument is greater than the second (where
``less than'' and ``greater than'' refer to whatever arbitrary
ordering is appropriate).

RETURNS
Returns a pointer to an element of <[array]> that matches <[key]>.  If
more than one matching element is available, the result may point to
any of them. Returns NULL if no matching element is found.

PORTABILITY
<<bsearch>> is ANSI.

No supporting OS subroutines are required.
*/

/**
searches an array beginning at <[base]> for any element
that matches <[key]>, using binary search
@return a pointer to an element of <[array]> that matches <[key]>.  If
more than one matching element is available, the result may point to
any of them. Returns NULL if no matching element is found.
@param key
@param base
@param nmemb
@param size
*/
void* bsearch (const void* key, const void* base, TUint32 nmemb, TUint32 size,
	int (*compar)(const void*, const void*))
	{
	TAnsiKey searchMe(key, base, size, compar);
	TInt result=KIndexPtr;
	TInt r=User::BinarySearch(nmemb, searchMe, result);
	if (r==0)
		return searchMe.At(result);
	else
		return NULL;
	}

/*
FUNCTION
<<qsort>>---sort an array

INDEX
	qsort

ANSI_SYNOPSIS
	#include <stdlib.h>
	void qsort(void *<[base]>, size_t <[nmemb]>, size_t <[size]>,
		   int (*<[compar]>)(const void *, const void *) );

TRAD_SYNOPSIS
	#include <stdlib.h>
	qsort(<[base]>, <[nmemb]>, <[size]>, <[compar]> )
	char *<[base]>;
	size_t <[nmemb]>;
	size_t <[size]>;
	int (*<[compar]>)();

DESCRIPTION
<<qsort>> sorts an array (beginning at <[base]>) of <[nmemb]> objects.
<[size]> describes the size of each element of the array.

You must supply a pointer to a comparison function, using the argument
shown as <[compar]>.  (This permits sorting objects of unknown
properties.)  Define the comparison function to accept two arguments,
each a pointer to an element of the array starting at <[base]>.  The
result of <<(*<[compar]>)>> must be negative if the first argument is
less than the second, zero if the two arguments match, and positive if
the first argument is greater than the second (where ``less than'' and
``greater than'' refer to whatever arbitrary ordering is appropriate).

The array is sorted in place; that is, when <<qsort>> returns, the
array elements beginning at <[base]> have been reordered.

RETURNS
<<qsort>> does not return a result.

PORTABILITY
<<qsort>> is required by ANSI (without specifying the sorting algorithm).
*/

/**
Sort an array.
@param base 
@param nmemb
@param size describes the size of each element of the array
*/
void qsort (void* base, TUint32 nmemb, TUint32 size,
	int (*compar)(const void*, const void*))
	{
	TAnsiKey  searchMe(NULL, base, size, compar);
	TAnsiSwap swapUs(base,size);
	User::QuickSort(nmemb, searchMe, swapUs);
	return;
	}

