// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\resourcecontrol.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/* The Driver's Version */
inline TVersion DResConPddFactory::VersionRequired()
	{
	const TInt KResConMajorVersionNumber=1;
	const TInt KResConMinorVersionNumber=0;
	const TInt KResConBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KResConMajorVersionNumber,KResConMinorVersionNumber,KResConBuildVersionNumber);
	}

/** Second stage constructor
 Allocates the specified size in kernel heap and creates a virtual link */
template <class T>
inline TInt DResourceCon<T>::Initialise(TInt aInitialSize)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::Initialise"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aInitialSize %d", aInitialSize));
    //Allocate memory for size specified.
	if(aInitialSize != 0)
		{
		iArray = (T**) new (T*[aInitialSize]);
		if((!iArray))
			{
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("No Sufficient memory for iArray allocation"));
			return KErrNoMemory;
			}
		//Create a virtual link by storing in each array position the index of next higher free location
		for(TInt c = 0; c < aInitialSize; c++)
			iArray[c] = (T*)(c+1);
		}
    iAllocated = (TUint16)aInitialSize;
    iGrowBy = (TUint16) (aInitialSize < 2 ? aInitialSize : aInitialSize/2);
    iCount = 0;
    iInstanceCount = 0;
    iFreeLoc = 0;
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DResourceCon<T>::Initialise"));
#ifdef PRM_INSTRUMENTATION_MACRO
	if(aInitialSize)
		{
		TUint size = aInitialSize * 4;
		PRM_MEMORY_USAGE_TRACE
		}
#endif
	return KErrNone;
	}

/** Resize the array */
template <class T>
inline TInt DResourceCon<T>::ReSize(TInt aGrowBy)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::ReSize"));
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("aGrowBy %d\n", aGrowBy));
	// Allocate memory for already existing + new required size.
	TInt r = Kern::SafeReAlloc((TAny*&)iArray, iAllocated * sizeof(T*), (iAllocated+aGrowBy)*sizeof(T*));
	if(r != KErrNone)
		return r;
    TInt c = iAllocated;
    //Virtually link the free ones
    while(c<(iAllocated+aGrowBy))
		{
         iArray[c]=(T*)(c+1);
         c++;
		}
    iAllocated = TUint16(iAllocated + aGrowBy);
#ifdef PRM_INSTRUMENTATION_MACRO
    TUint size = aGrowBy*4;
    PRM_MEMORY_USAGE_TRACE
#endif
	__KTRACE_OPT(KRESMANAGER, Kern::Printf("< DResourceCon<T>::ReSize, iAllocated = %d", iAllocated));
	return KErrNone;
	}

/** Delete the allocated array */
template <class T>
inline void DResourceCon<T>::Delete()
	{
    delete []iArray;
    iArray = NULL;
	}

/** Find the object at the specified location */
template <class T>
inline T* DResourceCon<T>::operator[](TInt anIndex)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::operator[], anIndex = %d", anIndex));
	// Check if passed index is inside allocated range and is not free.
    if(anIndex>=iAllocated || (((TUint)iArray[anIndex])<=iAllocated))
		return NULL;
    return iArray[anIndex];
	}

/** Remove the specified object from the container */
template <class T>
inline TInt DResourceCon<T>::Remove(T* /*aObj */, TInt aIndex)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::Remove"));
    if(aIndex>=(TInt)iAllocated)
		{
		__KTRACE_OPT(KRESMANAGER, Kern::Printf("Object not found, iAllocated = %d, index = %d", iAllocated, aIndex));
		DPowerResourceController::Panic(DPowerResourceController::EObjectNotFoundInList);
		}
	// Add the entry to the free location
	iArray[aIndex] = (T*)iFreeLoc;
	iFreeLoc = (TUint16)aIndex;
    iCount--; //Decrement valid client count
	__KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::Remove"));
    return KErrNone;
	}

/** Add the specified object into the container */
template <class T>
inline TInt DResourceCon<T>::Add(T* aObj, TUint &aId)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::Add"));
	//Check for array full
	if(iFreeLoc == iAllocated)
		return KErrNoMemory;
    //Update in the array in the free location
	aId = ((++iInstanceCount & INSTANCE_COUNT_BIT_MASK) << INSTANCE_COUNT_POS); //Instance count
	aId |= (iFreeLoc & ID_INDEX_BIT_MASK); //Array index
    TUint nextFreeLoc = (TUint)iArray[iFreeLoc];
    iArray[iFreeLoc] = aObj;
    iFreeLoc = (TUint16)nextFreeLoc;
    __KTRACE_OPT(KRESMANAGER, Kern::Printf("iFreeLoc %d", iFreeLoc));
    iCount++;  //Increment the valid client count
    return KErrNone;
	}

/** Find the entry with specified name and update in anEntry */
template <class T>
inline TInt DResourceCon<T>::Find(T*& anEntry, TDesC8& aName)
	{
    __KTRACE_OPT(KRESMANAGER, Kern::Printf(">DResourceCon<T>::Find, aName %S", &aName));
    anEntry = NULL;
    T* pC=anEntry;
    for(TInt count = 0; count < (TInt)iAllocated; count++)
		{
        /* Check whether the location is free */
        if(((TUint)iArray[count]) <= iAllocated)
            continue;
        pC=(T*)iArray[count];
        if(!aName.Compare(*(const TDesC8*)pC->iName))
			{
			anEntry=pC;
			__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DResourceCon<T>::Find, Entry Found"));
			return KErrNone;
			}
		}

	__KTRACE_OPT(KRESMANAGER, Kern::Printf("<DResourceCon<T>::Find, Entry Not Found"));
    return KErrNotFound;
	}
