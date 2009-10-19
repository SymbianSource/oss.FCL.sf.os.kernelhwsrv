// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\usbcque.h
// Simple singly linked list + its iterator for the USB Device driver.
// 
//

/**
 @file usbcque.h
 @internalTechnology
*/

#ifndef __USBCQUE_H__
#define __USBCQUE_H__

#include <kernel/kernel.h>


//
// --- Class definitions ---
//

class TSglQueLink
	{
private:
	void Enque(TSglQueLink* aLink);
public:
	TSglQueLink* iNext;
	friend class TSglQueBase;
	};


class TSglQueBase
	{
protected:
	TSglQueBase(TInt aOffset);
	void DoAddLast(TAny* aPtr);
	void DoRemove(TAny* aPtr);
protected:
	TSglQueLink* iHead;
	TSglQueLink* iLast;
	TInt iOffset;
	TInt iElements;
private:
	friend class TSglQueIterBase;
	};


template<class T>
class TSglQue : public TSglQueBase
	{
public:
	inline TSglQue(TInt aOffset);
	inline void AddLast(T& aRef);
	inline void Remove(T& aRef);
	inline TInt Elements() const;
	};


class TSglQueIterBase
	{
public:
	void SetToFirst();
protected:
	TSglQueIterBase(TSglQueBase& aQue);
	TAny* DoPostInc();
	TAny* DoCurrent();
protected:
	TInt iOffset;
	TSglQueLink*& iHead;
	TSglQueLink* iNext;
	};


template<class T>
class TSglQueIter : public TSglQueIterBase
	{
public:
	inline TSglQueIter(TSglQueBase& aQue);
	inline operator T*();
	inline T* operator++(TInt);
	};

//
// --- Inline implementations ---
//

// Class TSglQue
template<class T>
inline TSglQue<T>::TSglQue(TInt aOffset)
	: TSglQueBase(aOffset)
	{}


template<class T>
inline void TSglQue<T>::AddLast(T& aRef)
	{
	DoAddLast(&aRef);
	}


template<class T>
inline void TSglQue<T>::Remove(T& aRef)
	{
	DoRemove(&aRef);
	}


template<class T>
inline TInt TSglQue<T>::Elements() const
	{
	return iElements;
	}


// Class TSglQueIter
template<class T>
inline TSglQueIter<T>::TSglQueIter(TSglQueBase& aQue)
	: TSglQueIterBase(aQue)
	{}


template<class T>
inline TSglQueIter<T>::operator T*()
	{
	return ((T*)DoCurrent());
	}

template<class T>
inline T* TSglQueIter<T>::operator++(TInt)
	{
	return ((T*)DoPostInc());
	}


#endif	// __USBCQUE_H__
