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
// f32\sfile\sf_pool.h
// 
//

#include "sf_std.h"

#ifndef SF_POOL_H_
#define SF_POOL_H_

/**
 * @internalTechnology
 */
template <class T>
class CFsPool
	{
public:
	static CFsPool<T>* New(TInt aPoolSize);
	~CFsPool();
	
	/*
	 * Allocate returns a pointer of class T.
	 * The pointer returned is removed from the pool.
	 * When the pool is empty this function will wait.
	 */
	T* Allocate();
	
	/*
	 * Following a call to Allocate,
	 * Free teturns an object-pointer of type T to the pool.
	 */
	void Free(T* aBlock);
private:
	CFsPool();
	TInt Construct(TInt aPoolSize);
	
	void Lock();
	void Unlock();
	RSemaphore iPoolLock;
		
	RPointerArray<T> iFreeList;
	};

#endif /* SF_POOL_H_ */
