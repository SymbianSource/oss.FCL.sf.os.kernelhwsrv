// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32/compsupp/symaehabi/symbian_rtti.cpp
// 
//

#include "cxxabi.h"
#include "unwind_env.h"
#include "unwinder.h"
#include "symbian_support.h"

// For information about these classes, se
// www.codesourcery.com/public/cxx-abi/abi.html#rtti. 
using abi::__class_type_info;
using abi::__si_class_type_info;
using abi::__vmi_class_type_info;
using abi::__base_class_type_info;


// Given base class info (aBaseInfo) and a derived object pointer (aDerivedObj),
// this function implements "cast to base" and stores result in aBaseObj. This
// essentially sets aBaseObj to point to a memory location that represents
// aDerivedObj as the given base class. This function requires that aBaseInfo
// describes a base class of the aDerivedObj's class.
static void _CastUp(const __base_class_type_info& aBaseInfo, TAny** aDerivedObj, TAny** aBaseObj)
	{
	// Guard against a null pointer for aDerivedObj
	if ( ! (*aDerivedObj) ) 
		{
		*aBaseObj = NULL;
		return;
		}

	TInt offset = aBaseInfo.__offset_flags >> aBaseInfo.__offset_shift;

	if (aBaseInfo.__offset_flags & aBaseInfo.__virtual_mask)
		{
		// For virtual bases, look up offset in vtable + offset.

		// Get vtbl pointer as the first 4 bytes of **aDerivedObj
		TUint32* vptr = (TUint32*) ( *( (TUint32*) (*aDerivedObj) ) );

		offset = *( vptr + offset / sizeof(TUint32) );
		}

	// Apply the offset.
	*aBaseObj = (TAny*) ( ( (TUint8*) *aDerivedObj ) + offset );
	}


// For a description of this function, see comments in unwind_env.h.
extern "C" TBool _DoDerivedToBaseConversion(const std::type_info* aDerivedType,
		                                    const std::type_info* aBaseType,
		                                    TAny**                aDerivedObj,
		                                    TAny**                aBaseObj)
	{

	const std::type_info& type_base_type = typeid(*aBaseType);
	const std::type_info& type_derived_type = typeid(*aDerivedType);

	// We must proceed depending on the type of the type_info objects for derived
	// class.  
	if ( type_derived_type == typeid(__si_class_type_info) )
		{
		// The __si_class_type_info means that the derived type has a single,
		// public, non-virtual base, and that the base is at offset zero. We should
		// be able to simply compare the base type from __si_class_type_info with
		// aBaseType

		const __si_class_type_info* derived = (const __si_class_type_info*) aDerivedType;

		if ( *(derived->__base_type) == *aBaseType ) 
			{
			// The types match, work done.
			*aBaseObj = *aDerivedObj;
			return true;
			} 
		else 
			{
			// The types don't match. We should proceed to comparison with the any
			// classes. In this case there is a single base as follows:
			const __class_type_info* bType = derived->__base_type;

			// No pointer adjustment required for __si_class_type_info. 
			return _DoDerivedToBaseConversion(bType, aBaseType, aDerivedObj, aBaseObj);
			}
		}
	else if ( type_derived_type == typeid(__vmi_class_type_info) ) 
		{
		// The __vmi_class_type_info is for all other scenarios. We get an array of
		// bases which we need to traverse and do comparison on.

		const __vmi_class_type_info* derived = (const __vmi_class_type_info*)aDerivedType;

		const unsigned count = derived->__base_count;

		for ( unsigned i = 0; i < count; i++ ) 
			{
			// Get the base info for this base class
			const __base_class_type_info bInfo = derived->__base_info[i];

			if ( ! ( bInfo.__offset_flags & bInfo.__public_mask) )
				{
				// The base is non-public base, so the remainder of the hierarchy
				// above this base is of no interest
				continue;
				} 

			// Get the class type info for this base
			const __class_type_info* bType = bInfo.__base_type;

			// First check if the type from the list corresponds to requested base
			// type.  
			if ( (*bType) == (*aBaseType) ) 
				{
				// Match! Convert the pointer to point to the base.
				_CastUp(bInfo, aDerivedObj, aBaseObj);
				return true;
				}

			// No match, we must now recursively delve into superclasses of bType.
			// To do that, we need to advance the derived class pointer to point to
			// the current base class as in bInfo.
			TAny* newDerivedPtr;
			_CastUp(bInfo, aDerivedObj, &newDerivedPtr);

			// Now go recursive, substituting aDerivedObj with adjusted pointer and bType instead of aDerivedType 
			TBool result = _DoDerivedToBaseConversion(bType, aBaseType, (TAny**)(&newDerivedPtr), aBaseObj);

			// Return only if the match is found, otherwise continue with the loop
			if ( result ) 
				{
				// Match came back from recursion, pass up.
				return true;
				}

			// No match from recursion, advance to next base.
			}
		}
	else
		{
		// assert(0);
		}

	// No match was found for aBaseType in the aDerivedType's ancestry.
	return false;
	}

