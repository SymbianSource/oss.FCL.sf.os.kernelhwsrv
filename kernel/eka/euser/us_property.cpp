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
// e32\euser\us_property.cpp
// 
//

#include "us_std.h"

/**
Defines a property with the specified category ID.  This method should only be
used to specify a category different from the creating process's secure ID in 
exceptional circumstances.  In most cases the overload:

RProperty::Define(TUint aKey, TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocate)

should be used.  For details see the document located at:

Symbian OS guide » Base » Using User Library (E32) » Publish and Subscribe » Security issues

Defines the attributes and access control for a property. This can only be done 
once for each property. Subsequent attempts to define the same property will return
KErrAlreadyExists.

Only processes with the write-system-data capability are allowed to define 
properties either in the system category (KUidSystemCategory) or with 
aCategory < KUidSecurityThresholdCategoryValue. Any attempt to define 
a property with these categories by a process with insufficient capabilities 
will fail with a KErrPermissionDenied error.

Following the property's definition, it will have a default value, 0 for integer
properties and zero-length data for byte-array and text properties.
Pending subscriptions for this property will not be completed until a new
value is published.

@param aCategory    The UID that identifies the property category.
					This must either be the current process's Secure ID, or
					KUidSystemCategoryValue.
@param aKey         The property sub-key, i.e. the key that identifies the
                    specific property within the category.
@param aAttr        This describes the property type, a TType value;
                    persistence, as defined by the KPersistent bit, may
                    be ORed in.
@param aReadPolicy	A security policy defining the security attributes a
					process must have in order to read this value.
@param aWritePolicy	A security policy defining the security attributes a
					process must have in order to write this value.
@param aPreallocate The number of bytes to be pre-allocated for variable
                    sized properties. Pre-allocating enough space ensures that
                    a variable sized property can be set in 'real-time', 
                    (i.e. the time to set the property is bounded). 

@return KErrNone, if successful;
        KErrArgument, if the wrong type or attribute was specified;
        KErrArgument, if aType is TInt and aPreallocate is not 0;
        KErrTooBig, if aPreallocate is greater than KMaxPropertySize;
        KErrPermissionDenied, if an attempt is made to define a property in
        the system category by a process with insufficient capabilities, or
		the category secified wasn't the same as the current process's Secure ID.

@capability WriteDeviceData if aCategory==KUidSystemCategoryValue.
@capability WriteDeviceData if aCategory not equal to the current process's
			Secure ID and aCategory<KUidSecurityThresholdCategoryValue.


@see KUidSecurityThresholdCategoryValue

@publishedPartner
@released
*/
EXPORT_C TInt RProperty::Define(TUid aCategory, TUint aKey, TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocate)
	{
	if(aPreallocate < 0)
		return KErrArgument;
	if(aPreallocate > KMaxLargePropertySize)
		return KErrTooBig;

	TPropertyInfo info;
	info.iType = (RProperty::TType)(aAttr & RProperty::ETypeMask);
	info.iAttr = (aAttr & ~RProperty::ETypeMask);
	info.iSize = (TUint16) aPreallocate;
	info.iReadPolicy = aReadPolicy;
	info.iWritePolicy = aWritePolicy;
	return(Exec::PropertyDefine(TUint(aCategory.iUid), aKey, &info));
	}


/**
Defines a property.

Defines the attributes and access control for a property. This can only be done 
once for each property. Subsequent attempts to define the same property will return
KErrAlreadyExists.

The category ID for the property will be the same as the current processes Secure ID.

Following the property's definition, it will have a default value, 0 for integer
properties and zero-length data for byte-array and text properties.
Pending subscriptions for this property will not be completed until a new
value is published.

@param aKey         The property sub-key, i.e. the key that identifies the
                    specific property within the category.
@param aAttr        This describes the property type, a TType value;
                    persistence, as defined by the KPersistent bit, may
                    be ORed in.
@param aReadPolicy	A security policy defining the security attributes a
					process must have in order to read this value.
@param aWritePolicy	A security policy defining the security attributes a
					process must have in order to write this value.
@param aPreallocate The number of bytes to be pre-allocated for variable
                    sized properties. Pre-allocating enough space ensures that
                    a variable sized property can be set in 'real-time', 
                    (i.e. the time to set the property is bounded). 

@return KErrNone, if successful;
        KErrArgument, if the wrong type or attribute was specified;
        KErrArgument, if aType is TInt and aPreallocate is not 0;
        KErrTooBig, if aPreallocate is greater than KMaxPropertySize;

@publishedPartner
@released
*/
EXPORT_C TInt RProperty::Define(TUint aKey, TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocate)
	{
	TUid category = {-1}; 
	return Define(category, aKey, aAttr, aReadPolicy, aWritePolicy, aPreallocate);
	}



/**
NOTE - The use of this method is deprecated.

Defines a property with the specified category ID.  This method should only be
used to specify a category different from the creating process's secure ID in 
exceptional circumstances.  In most cases the overload:

RProperty::Define(TUint aKey, TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocate)

should be used.  For details see the document located at:

Symbian OS guide » Base » Using User Library (E32) » Publish and Subscribe » Security issues

Defines the attributes and access control for a property. This can only be done
once for each property. Subsequent attempts to define the same property will 
return KErrAlreadyExists.

Only processes with the write-system-data capability are allowed to define 
properties either in the system category (KUidSystemCategory) or with 
aCategory < KUidSecurityThresholdCategoryValue. Any attempt to define 
a property with these categories by a process with insufficient capabilities 
will fail with a KErrPermissionDenied error.

Following the property's definition, it will have a default value, 0 for integer
properties and zero-length data for byte-array and text properties.
Pending subscriptions for this property will not be completed until a new
value is published.

@param aCategory    The UID that identifies the property category.
@param aKey         The property sub-key, i.e. the key that identifies the
                    specific property within the category.
@param aAttr        This describes the property type, a TType value;
                    persistence, as defined by the KPersistent bit, may
                    be ORed in.
@param aPreallocate The number of bytes to be pre-allocated for variable
                    sized properties. Pre-allocating enough space ensures that
                    a variable sized property can be set in 'real-time', 
                    (i.e. the time to set the property is bounded). 

@return KErrNone, if successful;
        KErrArgument, if the wrong type or attribute was specified;
        KErrArgument, if aType is TInt and aPreallocate is not 0;
        KErrTooBig, if aPreallocate is greater than KMaxPropertySize;
        KErrPermissionDenied, if an attempt is made to define a property in
        the system category by a process with insufficient capabilities. 

@capability WriteDeviceData if aCategory==KUidSystemCategoryValue.
@capability WriteDeviceData if aCategory not equal to the current process's
			Secure ID and aCategory<KUidSecurityThresholdCategoryValue.

@see KUidSecurityThresholdCategoryValue
@publishedAll
@deprecated Use RProperty::Define(TUint aKey, TInt aAttr, const TSecurityPolicy &aReadPolicy, const TSecurityPolicy &aWritePolicy, TInt aPreallocated=0)
			instead.
*/
EXPORT_C TInt RProperty::Define(TUid aCategory, TUint aKey, TInt aAttr, TInt aPreallocate)
	{
	TPropertyInfo info;
	info.iType = (RProperty::TType)(aAttr & RProperty::ETypeMask);
	info.iAttr = (aAttr & ~RProperty::ETypeMask);
	info.iSize = (TUint16) aPreallocate;
	info.iReadPolicy = TSecurityPolicy(TSecurityPolicy::EAlwaysPass);
	info.iWritePolicy = TSecurityPolicy(TSecurityPolicy::EAlwaysPass);
	return(Exec::PropertyDefine(TUint(aCategory.iUid), aKey, &info));
	}


/**
Deletes a property.

This can only be called by the property owner, as defined by
the process Security ID; any attempt by another process to delete
the property will fail.

Any pending subscriptions for this property will be completed
with KErrNotFound.
Any new request will not complete until the property is defined
and published again.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.

@return KErrNone, if successful;
        KErrPermissionDenied, if a process that is not the owner of
        the property attempts to delete it.
        KErrNotFound, if the property has not been defined.
*/
EXPORT_C TInt RProperty::Delete(TUid aCategory, TUint aKey)
	{
	return(Exec::PropertyDelete(TUint(aCategory.iUid), aKey));
	}



/**
Deletes a property.

The category ID for the property will be the same as the current processes Secure ID.

Any pending subscriptions for this property will be completed
with KErrNotFound.
Any new request will not complete until the property is defined
and published again.

@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.

@return KErrNone, if successful;
        KErrNotFound, if the property has not been defined.
*/
EXPORT_C TInt RProperty::Delete(TUint aKey)
	{
	return(Exec::PropertyDelete(KMaxTUint, aKey));
	}


/**
Gets an integer property.

The function gets the integer value of the specified property.

The Platform Security attributes of the current process are checked against
the Read Policy which was specified when the property was defined.
If this check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
check failed.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aValue    A reference to the variable where the property value will
                 be reported.

@return KErrNone, if successful;
	    KErrPermissionDenied, if the caller process doesn't pass the Read Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not of integral type.
*/
EXPORT_C TInt RProperty::Get(TUid aCategory, TUint aKey, TInt& aValue)
	{
	return(Exec::PropertyFindGetI(TUint(aCategory.iUid), aKey, &aValue));
	}




/**
Gets a binary property.

The function gets the byte-array (binary) value of the specified property.

The Platform Security attributes of the current process are checked against
the Read Policy which was specified when the property was defined.
If this check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
check failed.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aDes      A reference to the buffer descriptor where the property value
                 will be reported.

@return KErrNone if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Read Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a byte-array (binary) type;
		KErrOverflow, if the supplied buffer is too small to contain the full
		property value, and note that the buffer aDes contains the
		truncated property value.
*/
EXPORT_C TInt RProperty::Get(TUid aCategory, TUint aKey, TDes8& aDes)
	{
	TInt size = aDes.MaxSize();
	TInt r = Exec::PropertyFindGetB(TUint(aCategory.iUid), aKey, (TUint8*) aDes.Ptr(), size);
	if (r < 0)
		{
		if (r == KErrOverflow)
			{
			aDes.SetLength(size);
			}
		return r;
		}
	aDes.SetLength(r);
	return KErrNone;
	}




/**
Gets a text property.

The function gets the text value of the specified property.

The Platform Security attributes of the current process are checked against
the Read Policy which was specified when the property was defined.
If this check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
check failed.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aDes      A reference to the buffer descriptor where the property value
                 will be reported.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Read Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a text type;
		KErrOverflow, if the supplied buffer is too small to contain the full
		property value, and note that the buffer aDes contains the
		truncated property value.
*/
EXPORT_C TInt RProperty::Get(TUid aCategory, TUint aKey, TDes16& aDes)
	{
	TInt size = aDes.MaxSize();
	TInt r = Exec::PropertyFindGetB(TUint(aCategory.iUid), aKey, (TUint8*) aDes.Ptr(), size);
	if (r < 0)
		{
		if (r == KErrOverflow)
			{
			aDes.SetLength(size >> 1);
			}
		return r;
		}
	aDes.SetLength(r >> 1);
	return KErrNone;
	}




/**
Sets an integer property.

The function publishes a new integral property value.

Any pending subscriptions for this property will be completed.

The Platform Security attributes of the current process are checked against
the Write Policy which was specified when the property was defined.
If this check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
check failed.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aValue    The new property value. 

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Write Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not of integral type.
*/
EXPORT_C TInt RProperty::Set(TUid aCategory, TUint aKey, TInt aValue)
	{
	return(Exec::PropertyFindSetI(TUint(aCategory.iUid), aKey, aValue));
	}




/**
Sets a binary property.

The function Publishes a new byte-array (binary) value for
the specified property.

Any pending subscriptions for this property will be completed.

Note that if the new property value requires more storage space than is
currently allocated, then memory allocation will be required.
This invalidates any real-time guarantee, i.e. the guarantee that the operation
will complete within a bounded time.

The Platform Security attributes of the current process are checked against
the Write Policy which was specified when the property was defined.
If this check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
check failed.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aDes      A reference to the descriptor containing the
                 new property value.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Write Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a byte-array (binary) type;
	    KErrNoMemory, if memory allocation is required, and there is
	                  insufficient available.
*/
EXPORT_C TInt RProperty::Set(TUid aCategory, TUint aKey, const TDesC8& aDes)
	{
	return(Exec::PropertyFindSetB(TUint(aCategory.iUid), aKey, (TUint8*) aDes.Ptr(), aDes.Size()));
	}




/**
Sets a text property.

The function publishes a new text value for the specified property.

Any pending subscriptions for this property will be completed.

Note that if the new property value requires more storage space than is
currently allocated, then memory allocation will be required.
This invalidates any real-time guarantee, i.e. the guarantee that the operation
will complete within a bounded time.

The Platform Security attributes of the current process are checked against
the Write Policy which was specified when the property was defined.
If this check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return KErrNone even though the
check failed.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aDes      A reference to the descriptor containing the
                 new property value.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Write Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a text type;
	    KErrNoMemory, if memory allocation is required, and there is
	                  insufficient available;
        KErrTooBig, if the property is larger than KMaxPropertySize;
*/
EXPORT_C TInt RProperty::Set(TUid aCategory, TUint aKey, const TDesC16& aDes)
	{
	return(Exec::PropertyFindSetB(TUint(aCategory.iUid), aKey, (TUint8*) aDes.Ptr(), aDes.Size()));
	}




/**
Attaches to the specified property.

The function creates a handle (this object) to the specified property.
This allows the caller to subscribe for notification of changes to this
property, and to faster and real-time property access methods.

If the specified property does not exist, then this operation will
still succeed. However, memory allocation will be required.
Note that this invalidates any real-time guarantee, i.e. the guarantee that
the operation completes within a bounded time.

@param aCategory The UID that identifies the property category.
@param aKey      The property sub-key, i.e. the key that identifies the
                 specific property within the category.
@param aType     The ownership of this property handle. 
				 By default, ownership is vested in the current process,
				 but can be vested in the current thread by
				 specifying EOwnerThread.

@return KErrNone, if successful;
	    KErrNoMemory, if memory allocation is required, and there is
	                  insufficient available.
*/
EXPORT_C TInt RProperty::Attach(TUid aCategory, TUint aKey, TOwnerType aType)
	{
	TInt r = Exec::PropertyAttach(TUint(aCategory.iUid), aKey, aType);
	if (r < 0) 
		{ // error
		iHandle = 0;
		return r;
		}
	iHandle = r;
	return KErrNone;
	}




/**
Subscribes to a property.

The function issues an asynchronous request to be notified when the property
is changed. The calling thread is signalled, and the specified request status
object is updated when the property is next changed.

The property may change several times before the subscribing thread can deal
with a notification request completion. To ensure that the subscriber does not
miss updates, it should re-issue a subscription request before retrieving
the current value and acting on it.

If the property has not been defined, the request does not complete until
the property is subsequently defined and published. When defined, if the caller
process doesn't pass the Read Policy, then the request completes with KErrPermissionDenied.

If the property is already defined, and the caller process doesn't pass the Read Policy,
then the request completes immediately with KErrPermissionDenied.

When Read Policy checks fail the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then the request will complete successfully even though the
check failed.

If an outstanding request is cancelled through a call to Cancel(), then it
completes with KErrCancel.

@param aRequest The request status object to be signalled on update.

@panic KERN-EXEC 9 if there is already a subscription on this property handle;
       only one subscription per RProperty is allowed.
*/
EXPORT_C void RProperty::Subscribe(TRequestStatus& aRequest)
	{
	aRequest = KRequestPending;
	Exec::PropertySubscribe(iHandle, &aRequest);
	}




/**
Cancels an outstanding subscription request for this property handle.

If the request has not already completed, then it completes with KErrCancel.
*/
EXPORT_C void RProperty::Cancel()
	{
	Exec::PropertyCancel(iHandle);
	}




/**
Gets the integer value of this property.

The implementation guarantees that this call has a bounded response time.

@param aValue A reference to the variable where the property value
              will be reported.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Read Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not of integral type.
*/
EXPORT_C TInt RProperty::Get(TInt& aValue)
	{
	return(Exec::PropertyGetI(iHandle, &aValue));
	}




/**
Gets the byte-array (binary) value of this property.

The implementation guarantees that this call has a bounded response time.

@param aDes A reference to the buffer descriptor where the property value
            will be reported.
            
@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Read Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a byte-array (binary) type.
		KErrOverflow, if the supplied buffer is too small to contain the full
		property value, and note that the buffer aDes contains the
		truncated property value.
*/
EXPORT_C TInt RProperty::Get(TDes8& aDes)
	{
	TInt size = aDes.MaxSize();
	TInt r = Exec::PropertyGetB(iHandle, (TUint8*) aDes.Ptr(), size);
	if (r < 0)
		{
		if (r == KErrOverflow)
			{
			aDes.SetLength(size);
			}
		return r;
		}
	aDes.SetLength(r);
	return KErrNone;
	}




/**
Gets the text value of this property.

The implementation guarantees that this call has a bounded response time.

@param aDes A reference to the buffer descriptor where the property value
            will be reported.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Read Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a text type;
		KErrOverflow, if the supplied buffer is too small to contain the full
		property value, and note that the buffer aDes contains the
		truncated property value.
*/
EXPORT_C TInt RProperty::Get(TDes16& aDes)
	{
	TInt size = aDes.MaxSize();
	TInt r = Exec::PropertyGetB(iHandle, (TUint8*) aDes.Ptr(), size);
	if (r < 0)
		{
		if (r == KErrOverflow)
			{
			aDes.SetLength(size >> 1);
			}
		return r;
		}
	aDes.SetLength(r >> 1);
	return KErrNone;
	}




/**
Sets a new integer value for this property.

The function publishes the attached new integral property value, and any
pending subscriptions for this property are completed.

The implementation guarantees that this call has a bounded response time.

@param aValue The property new value.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Write Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not of integral type.
*/	
EXPORT_C TInt RProperty::Set(TInt aValue)
	{
	return(Exec::PropertySetI(iHandle, aValue));
	}




/**
Sets the byte-array (binary) property.

The function publishes the attached new binary property value, and any
pending subscriptions for this property are completed.

The implementation guarantees that this call has a bounded response time only
if the new property value requires no more storage space than is
currently allocated. If more memory needs to be allocated, then this
invalidates the real-time guarantee.

@param aDes A reference to the descriptor containing the property new value.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Write Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a byte-array (binary) type;
	    KErrNoMemory, if memory allocation is required, and there is
	                  insufficient available.
    	KMaxPropertySize, if the property is larger than KErrTooBig.
*/
EXPORT_C TInt RProperty::Set(const TDesC8& aDes)
	{
	return(Exec::PropertySetB(iHandle, (TUint8*) aDes.Ptr(), aDes.Size()));
	}




/**
Sets the text property

The function publishes the attached new text property value, and any
pending subscriptions for this property are completed.

The implementation guarantees that this call has a bounded response time only
if the new property value requires no more storage space than is
currently allocated. If more memory needs to be allocated, then this
invalidates the real-time guarantee.

@param aDes A reference to the descriptor containing the property new value.

@return KErrNone, if successful;
        KErrPermissionDenied, if the caller process doesn't pass the Write Policy;
		KErrNotFound, if the property has not been defined;
		KErrArgument, if the property is not a byte-array (binary) type;
	    KErrNoMemory, if memory allocation is required, and there is
	                  insufficient available.
    	KMaxPropertySize, if the property is larger than KErrTooBig.
*/
EXPORT_C TInt RProperty::Set(const TDesC16& aDes)
	{
	return(Exec::PropertySetB(iHandle, (TUint8*) aDes.Ptr(), aDes.Size()));
	}
