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
// e32\klib\dbase.cpp
// 
//

#include <kernel/kern_priv.h>

/**	Deletes the specified DBase derived object.

@param aPtr Pointer to the DBase derived object to be deleted.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C void DBase::Delete(DBase* aPtr)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DBase::Delete");	
	delete aPtr;
	}


/**	Allocates the object from the kernel heap and then initialises its contents
	to binary zeros.

@param aSize The size of the derived class. This parameter is specified
             implicitly by C++ in all circumstances in which a derived class
             is allocated.

@return An untyped pointer to the allocated object.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TAny* DBase::operator new(TUint aSize) __NO_THROW
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DBase::operator new(TUint aSize)");	
	return Kern::AllocZ(aSize);
	}


/**	Allocates the object from the kernel heap with additional memory and then
	initialises its contents to binary zeros.

@param  aSize The size of the derived class. This parameter is specified
              implicitly by C++ in all circumstances in which a derived class
              is allocated.
              
@param  anExtraSize Indicates additional size beyond the end of the base class.

@return An untyped pointer to the allocated object.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
EXPORT_C TAny* DBase::operator new(TUint aSize, TUint anExtraSize) __NO_THROW
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DBase::operator new(TUint aSize, TUint anExtraSize)");	
	aSize+=anExtraSize;
	return Kern::AllocZ(aSize);
	}

_LIT(KLitKernLib,"KernLib");
void KL::Panic(KL::TKernLibPanic aPanic)
	{
	Kern::PanicCurrentThread(KLitKernLib,aPanic);
	}


/** Default constructor for version type
	Sets version to 0.0.0
 */
EXPORT_C TVersion::TVersion()
	: iMajor(0),iMinor(0),iBuild(0)
	{}


/**
Compares two versions and returns true if the test version is less than the
current version.

Version information is encapsulated by a TVersion type object and consists of
a major version number, a minor version number and a build number.

The function returns true if one of the following conditions is true:

1. the test major version is strictly less than the current major version

2. the test major version is equal to the current major version and the test
   minor version is less than or equal to the current minor version.

If neither condition is true, the function returns false.

@param aCurrent   A reference to the current version against which aRequested
                  is compared.
@param aRequested A reference to the test version to be compared
                  against aCurrent.

@return True, if one or both conditions are true. False otherwise.
*/
EXPORT_C TBool Kern::QueryVersionSupported(const TVersion &aCurrent,const TVersion &aRequested)
	{

	if (aRequested.iMajor<aCurrent.iMajor || (aRequested.iMajor==aCurrent.iMajor && aRequested.iMinor<=aCurrent.iMinor))
		return(ETrue);
	return(EFalse);
	}


/** Constructor for version type.

	@param	aMajor	The major version number (0-127).
	@param	aMajor	The minor version number (0-127).
	@param	aMajor	The build number (0-32767).
 */
EXPORT_C TVersion::TVersion(TInt aMajor,TInt aMinor,TInt aBuild)
	: iMajor((TInt8)aMajor), iMinor((TInt8)aMinor), iBuild((TInt16)aBuild)
	{}


/** Converts a version type to a text string.

	The string is of the form X.YY(Z)
	where X is major version number, Y is minor and Z is build number.

	@return The string in a TBuf class.
 */
EXPORT_C TVersionName TVersion::Name() const
	{

	TVersionName v;
	v.AppendNum(iMajor);
	v.Append(TChar('.'));
	v.AppendNumFixedWidth(iMinor,EDecimal,2);
	v.Append(TChar('('));
	v.AppendNum(iBuild);
	v.Append(TChar(')'));
	return v;
	}

