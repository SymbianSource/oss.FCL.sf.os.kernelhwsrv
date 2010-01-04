/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



#if (!defined __FILESERVERUTIL_H__)
#define __FILESERVERUTIL_H__


//	EPOC Includes
#include <test/datawrapper.h>

/** 
@publishedAll
@released

Template class CleanupResetAndDestroy to clean up the array
of implementation information from the cleanup stack.
*/

template <class T>
class CleanupResetAndDestroy
	{
public:
	/**
	Puts an item on the cleanup stack.

	@param  aRef 
	        The implementation information to be put on the cleanup stack.
	*/
	inline static void PushL(T& aRef);
private:
	static void ResetAndDestroy(TAny *aPtr);
	};
template <class T>
inline void CleanupResetAndDestroyPushL(T& aRef);
template <class T>
inline void CleanupResetAndDestroy<T>::PushL(T& aRef)
	{CleanupStack::PushL(TCleanupItem(&ResetAndDestroy,&aRef));}
template <class T>
void CleanupResetAndDestroy<T>::ResetAndDestroy(TAny *aPtr)
	{(STATIC_CAST(T*,aPtr))->ResetAndDestroy();}
template <class T>
inline void CleanupResetAndDestroyPushL(T& aRef)
	{CleanupResetAndDestroy<T>::PushL(aRef);}

class FileserverUtil
	{
public:

	static TBool 		GetAttMask(CDataWrapper& aDataWrapper, const TDesC& aSection, const TDesC& aParameterName, TUint& aAttMask);
	static TBool		VerifyTEntryDataFromIniL(CDataWrapper& aDataWrapper, const TDesC& aSection, TEntry& aEntry);
	static TBool		VerifyTVolumeInfoDataFromIniL(CDataWrapper& aDataWrapper, const TDesC& aSection, TVolumeInfo& aVolumeInfo);
	
private:
	static TBool 		ConvertToAttMask(const TDesC& aAttMaskStr, TUint& aAttMask);
	};

#endif /* __FILESERVERUTIL_H__ */
