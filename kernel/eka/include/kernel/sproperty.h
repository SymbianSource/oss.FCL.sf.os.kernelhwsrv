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
// \e32\include\kernel\sproperty.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __SPROPERTY_H__
#define __SPROPERTY_H__

#include <nklib.h>

class DProcess;
class TProperty;




/**
@publishedPartner
@released

Defines the type of the function that will be called when a
request to be notified of the publication of a property completes.

A function of this type is passed via a TPropertySubsRequest object.

@see RPropertyRef::Subscribe()
@see TPropertySubsRequest
*/
typedef void (*TPropertyCompleteFn)(TAny* aPtr, TInt aReason);




/**
@publishedPartner
@released

Represents a pending request to be notified of the publication
of a property.

A pending request is also known as a pending subscription.

An object of this type is passed to RPropertyRef::Subscribe() when making
a request to be notified when a property is published.

The same object is also passed to RPropertyRef::Cancel() if the outstanding
subscription request is to be cancelled.

@see RPropertyRef::Subscribe()
@see RPropertyRef::Cancel()
*/
class TPropertySubsRequest : 
			public SDblQueLink	// protected by the system lock 
	{
public:
    /**
    Constructor.
    
    @param aCompleteFn The function that will be called when the subscription
                       request completes.
    @param aPtr        A pointer that is passed to the request completion
                       function.
    */ 
	TPropertySubsRequest(TPropertyCompleteFn aCompleteFn, TAny* aPtr)
		{
		iNext = NULL;
		iCompleteFn = aCompleteFn;
		iPtr = aPtr;
		}
    
    /**
    The function that will be called when the subscription request completes.
    */
	TPropertyCompleteFn	iCompleteFn;
	
	/**
	A pointer that is passed to the request completion function.
	*/
	TAny*				iPtr;

private:
	friend class TProperty;

	/**
    Defines the context of iProcess.
    @internalComponent
    */
	enum {KScheduledForCompletion = 1, KProcessPtrMask = 0xfffffffe};

    /**
    The process that has required notification.
    LSB is set if the request is scheduled for completition.
    */
    DProcess*   iProcess;

	};




/**
@publishedPartner
@released

An object that encapsulates information about a property.

An object of this type is passed to RPropertyRef::GetStatus(), which
fills in the fields.

@see RPropertyRef::GetStatus()
*/
class TPropertyStatus
	{
public:
	
	/**
	Additional property attributes.
	
	These are not currently defined, and are reserved for future use.
	*/
	TUint					iAttr;
	
	/**
	Byte-array size, in bytes, for
	property types: EByteArray and ELargeByteArray.
	
	@see RProperty
	*/
	TUint16					iSize;
	
	/**
	The type of the property.
	*/
	RProperty::TType		iType;
	
	/**
	The owner of the property.
	
	This is a pointer to a DProcess object, and represents the process that
	was current when the property was defined.
	
	@see RPropertyRef::Define()
	*/
	TUint32					iOwner;
	};




/**
@publishedPartner
@released

A reference to a property.

The class provides the interface to a property for code
running kernel side. 
*/
class RPropertyRef
	{
public:
    /**
    Default constructor.
    */
	RPropertyRef()
		{
		iProp = NULL;
		}

	IMPORT_C TInt Attach(TUid aCategory, TInt aKey);
	IMPORT_C TInt Open(TUid aCategory, TInt aKey);
	IMPORT_C void Close();

	IMPORT_C TInt Define(TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocate=0, DProcess* aProcess = NULL);
	IMPORT_C TInt Delete(DProcess* aProcess = NULL);

	IMPORT_C TInt Subscribe(TPropertySubsRequest& aRequest, DProcess* aProcess = NULL);
	IMPORT_C void Cancel(TPropertySubsRequest& aRequest);

	IMPORT_C TInt Get(TInt& aValue, DProcess* aProcess = NULL);
	IMPORT_C TInt Set(TInt aValue, DProcess* aProcess = NULL);
	IMPORT_C TInt Get(TDes8& aDes, DProcess* aProcess = NULL);
	IMPORT_C TInt Set(const TDesC8& aDes, DProcess* aProcess = NULL);

	IMPORT_C TBool GetStatus(TPropertyStatus& aStatus);

private:
	TProperty*	iProp;
	};

/**
@internalTechnology
*/
TInt PubSubPropertyInit();

#endif
