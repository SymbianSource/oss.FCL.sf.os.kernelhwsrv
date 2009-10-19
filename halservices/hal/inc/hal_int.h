// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// hal\inc\hal_int.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __HAL_INT_H__
#define __HAL_INT_H__

#include <hal.h>
#include <e32svr.h>

// Work around data import/export restriction of X86 compilers
#if defined(__X86__) || defined(__WINS__)
#undef IMPORT_D
#undef EXPORT_D
#define IMPORT_D IMPORT_C
#define EXPORT_D
#endif

/**
@publishedPartner
@released

The signature for accessor functions for derived attributes

@param aDeviceNumber A device number is applicable when a system has
multiple instances of a device. The parameter specifies which one to
interrogate. Eg. a phone may have 2 displays, so it's HAL accessor functions
would access different information depending on whether aDeviceNumber was 0 or 1.

@param aAttrib The HAL attribute to access.
@param aSet ETrue if the specified attribute should be modified; EFalse for a read.
@param aInOut A pointer to a TInt. If aSet is:
	- ETrue it points to the new value to be written.
	- EFalse it is used to return the value read.
	  It may also be used to pass an argument in,
	  in order to select one of multiple values to retreive.
	  If it is equal to -1 then the function must
	  return KErrArgument so that callers can identify that
	  the function uses aInOut as an input even when aSet is false.

@return An error code
	- KErrNone
	- KErrArgument aInOut was invalid (may occur for aSet true or false)
*/
typedef TInt (*THalImplementation)(TInt aDeviceNumber, TInt aAttrib, TBool aSet, TAny* aInOut);

/**
@internalComponent
*/
class HalInternal
	{
public:
	static const TUint8 Properties[HAL::ENumHalAttributes];
	static const TInt Offset[HAL::ENumHalAttributes];
	// InitialValue[] is only exported for patchdata purposes (other executables
	// must not import this array; all access should be through the published Hal
	// APIs).  IMPORT_D needs to be on the declaration here so that it has external
	// linkage (class data is treated differently to non-class data).
	IMPORT_D static const TInt InitialValue[HAL::ENumHalAttributes];
	static const THalImplementation Implementation[HAL::ENumHalAttributes];
	static const TInt HalDataSize;

	enum THalPanic
		{
		EReadOffsetInvalid=0,
		EWriteOffsetInvalid=1,
		EInitialAllocFailed1=2,
		EInitialAllocFailed2=3,
		EInitialWriteFailed=4,
		EInitialWriteFailed2=5,
		ETlsSizeInvalid=6,
		ENoSuchHalProp=7,
		EGetPropFailed=8,
		ESetPropFailed=9
		};

	static void Panic(THalPanic aPanic);
	static void InitialiseData();
	static TInt ReadWord(TInt anOffset);
	static TInt WriteWord(TInt anOffset, TInt aValue);
	};

#endif
