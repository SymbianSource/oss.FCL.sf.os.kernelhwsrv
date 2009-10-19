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
// e32\include\e32property.h
// 
//

#ifndef __E32PROPERTY_H__
#define __E32PROPERTY_H__

#include <e32cmn.h>

/**
@publishedAll
@released

Property category UID value reserved for System services.
*/
static const TInt32 KUidSystemCategoryValue=0x101f75b6;


/**
@publishedAll
@released

Property category UID reserved for System services
*/
static const TUid KUidSystemCategory={KUidSystemCategoryValue};


/**
@publishedAll
@released

The lowest value for Property categories at which additional
security restrictions are applied when defining properties.

Properties with category values above this threshold may only be defined
if the category matches the defining process's Secure ID.

Below this threashold, properties may be defined either by processes with
a matching Secure ID, or by processes with the WriteDeviceData capability.
*/
static const TInt32 KUidSecurityThresholdCategoryValue=0x10273357;


/**
@publishedAll
@released

User side interface to Publish & Subscribe.

The class defines a handle to a property, a single data value representing
an item of state information. Threads can publish (change) a property value
through this handle. Threads can also subscribe
(request notification of changes) to a property value through this handle;
they can also retrieve the current property value.
*/
class RProperty : public RHandleBase
	{
public:
	/**
	The largest supported property value, in bytes, for byte-array (binary)
	types and text types.
    */
	enum { KMaxPropertySize = 512 };
	/**
	The largest supported property value, in bytes, for large byte-array (binary)
	types and large text types.
    */
	enum { KMaxLargePropertySize = 65535 };


	/**
	Property type attribute.
	*/
	enum TType
		{
		/**
		Integral property type.
		*/
		EInt,
		
		
		/**
		Byte-array (binary data) property type.
		This type provides real-time guarantees but is limited to a maximum size
		of 512 bytes.

		@see KMaxPropertySize 
		*/
		EByteArray,
		
		
		/**
		Text property type. 
		This is just a programmer friendly view of a byte-array property, and
		is implemented in the same way as EByteArray.
		*/
		EText = EByteArray,


		/**
		Large byte-array (binary data) property type.
		This type provides no real-time guarantees but supports properties
		of up to 65536 bytes.

		@see KMaxLargePropertySize 
		*/
		ELargeByteArray,
		
		
		/**
		Large text property type. 
		This is just a programmer friendly view of a byte-array property, and
		is implemented in the same way as EByteArray.
		*/
		ELargeText = ELargeByteArray,


		/**
		Upper limit for TType values.
		It is the maximal legal TType value plus 1.
		*/ 
		ETypeLimit,
		
		
		/**
		Bitmask for TType values coded within TInt attributes.
		*/ 
		ETypeMask = 0xff
		};


public:
	IMPORT_C static TInt Define(TUid aCategory, TUint aKey, TInt aAttr, TInt aPreallocate=0);
	IMPORT_C static TInt Define(TUid aCategory, TUint aKey, TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocated=0);
	IMPORT_C static TInt Define(TUint aKey, TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocated=0);
	IMPORT_C static TInt Delete(TUid aCategory, TUint aKey);
	IMPORT_C static TInt Delete(TUint aKey);
	IMPORT_C static TInt Get(TUid aCategory, TUint aKey, TInt& aValue);
	IMPORT_C static TInt Get(TUid aCategory, TUint aKey, TDes8& aValue);
#ifndef __KERNEL_MODE__
	IMPORT_C static TInt Get(TUid aCategory, TUint aKey, TDes16& aValue);
#endif
	IMPORT_C static TInt Set(TUid aCategory, TUint aKey, TInt aValue);
	IMPORT_C static TInt Set(TUid aCategory, TUint aKey, const TDesC8& aValue);
#ifndef __KERNEL_MODE__
	IMPORT_C static TInt Set(TUid aCategory, TUint aKey, const TDesC16& aValue);
#endif

	IMPORT_C TInt Attach(TUid aCategory, TUint aKey, TOwnerType aType = EOwnerProcess);

	IMPORT_C void Subscribe(TRequestStatus& aRequest);
	IMPORT_C void Cancel();

	IMPORT_C TInt Get(TInt& aValue);
	IMPORT_C TInt Get(TDes8& aValue);
#ifndef __KERNEL_MODE__
	IMPORT_C TInt Get(TDes16& aValue);
#endif
	IMPORT_C TInt Set(TInt aValue);
	IMPORT_C TInt Set(const TDesC8& aValue);
#ifndef __KERNEL_MODE__
	IMPORT_C TInt Set(const TDesC16& aValue);
#endif
	};

#endif
