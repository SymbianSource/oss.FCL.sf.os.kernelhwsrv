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
// e32\include\e32base_private.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef __E32BASE_PRIVATE_H__
#define __E32BASE_PRIVATE_H__
#include <e32std.h>
#include <e32std_private.h>
#include <e32base.h>

/**
@internalComponent
*/
		struct	TObjectDataStr	//In use if this slot contains pointer to object.
			{			
			TInt16 instance;  
			TUint16 uniqueID;
			};
/**
@internalComponent
*/
		struct SObjectIxRec
			{
			union
			{
				TObjectDataStr str;	//This is in use if the slot contains pointer to CObject.	
				TInt nextEmpty;		//This is in use if the slot is empty. Points to the next empty slot ...
									//... in the list. 16 bits would be enough but ARM prefers 32 bytes.
			};
			CObject* obj;
			};

/**
@internalTechnology
@deprecated
*/
class CBitMapAllocator : public CBase
	{
public:
	IMPORT_C static CBitMapAllocator* New(TInt aSize);
	IMPORT_C static CBitMapAllocator* NewL(TInt aSize);
	IMPORT_C ~CBitMapAllocator();
	IMPORT_C TInt Alloc();
	IMPORT_C TInt AllocFrom(TInt aPos);
	IMPORT_C TInt Alloc(TInt aCount, TInt& aConsecutive);
	IMPORT_C TInt AllocAligned(TInt anAlignment);
	IMPORT_C TInt AllocAlignedBlock(TInt anAlignment);
	IMPORT_C TInt AllocFromTop();
	IMPORT_C TInt AllocFromTopFrom(TInt aPos);
	IMPORT_C void AllocAt(TInt aPos);
	IMPORT_C void AllocAt(TInt aPos, TInt aCount);
	IMPORT_C TBool IsFree(TInt aPos);
	IMPORT_C TBool IsFree(TInt aPos, TInt aCount);
	IMPORT_C void Free(TInt aPos);
	IMPORT_C void Free(TInt aPos, TInt aCount);
	IMPORT_C TInt Avail();
	IMPORT_C TInt Size();
	IMPORT_C TInt ExtractRamPages(TInt aConsecutive,TInt& aPageNo);
protected:
	IMPORT_C CBitMapAllocator(TInt aSize,TInt aLength);
protected:
	TInt iAvail;
	TInt iSize;
	TInt iLength;
	TUint iMap[1];
	};


#endif //__E32BASE_H__
