// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_pool.cpp
// 
//

#include "sf_pool.h"
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
#include "sf_notifier.h"
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	

//=====CFsPool=============================

template <class T>
CFsPool<T>* CFsPool<T>::New(TInt aPoolSize, T*(*aNewFunction)())
	{
	CFsPool<T>* pool = new CFsPool<T>();
	if(!pool)
		return NULL;
	
	TInt r = pool->Construct(aPoolSize,aNewFunction);
	if(r!=KErrNone)
		{
		delete pool;
		return NULL;
		}
	else
		return pool;
	}


template <class T>
CFsPool<T>::CFsPool()
	{
	}

template <class T>
TInt CFsPool<T>::Construct(TInt aPoolSize,T*(*aNewFunction)())
	{
	TInt r = iPoolLock.CreateLocal(aPoolSize);
	if(r != KErrNone)
			return r;
	
	r = iFreeList.Reserve(aPoolSize);
	if(r != KErrNone)
		return r;
	
	TInt i = 0;
	while(i < aPoolSize)
		{
		T* t = aNewFunction();
		if(!t)
			{
			return KErrNoMemory;
			}
		r = iFreeList.Append(t);
        if(r !=  KErrNone)
            {
            return r;
            }
		i++;
		}
	
	return KErrNone; 
	}

//This should only be called by the Manager when it holds
//the manager's write lock meaning that all of the
//blocks should be unallocated 
template <class T>
CFsPool<T>::~CFsPool()
	{
	for(TInt i=0; i < iFreeList.Count(); i++)
		{
		delete iFreeList[i];
		}
	iFreeList.Close();
	}


template <class T>
T* CFsPool<T>::Allocate()
	{
	Lock(); //Waits when there are no free blocks left

	TInt lastIndex = iFreeList.Count()-1;
	T* t = iFreeList[lastIndex];
	iFreeList.Remove(lastIndex);
	
	return t;
	}

template <class T>
void CFsPool<T>::Free(T* aBlock)
	{
	iFreeList.Append(aBlock);
	Unlock();
	}

template <class T>
void CFsPool<T>::Lock()
	{
	iPoolLock.Wait();
	}

template <class T>
void CFsPool<T>::Unlock()
	{
	iPoolLock.Signal();
	}

//These are needed here because the compiler needs to know which types will be 
//instantiating the template (because it's in a separate file)
#ifdef SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
template class CFsPool<CFsNotificationBlock>;
#endif //SYMBIAN_F32_ENHANCED_CHANGE_NOTIFICATION	
template class CFsPool<CFsNotificationInfo>;
