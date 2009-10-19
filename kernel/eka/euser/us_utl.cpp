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
// e32\euser\us_utl.cpp
// 
//

#include "us_std.h"


GLDEF_C void Panic(TCdtPanic aPanic)
//
// Panic the process with USER as the category.
//
	{

	User::Panic(_L("USER"),aPanic);
	}

EXPORT_C void User::PanicUnexpectedLeave()
	{
	::Panic(EUnexpectedLeave);
	}

EXPORT_C TFormatInfo::TFormatInfo()
//
// Constructor
//
	:iFormatIsCurrent(FALSE),i512ByteSectorsFormatted(0),iMaxBytesPerFormat(0)
	{}

#if defined(_UNICODE)
void TestOverflowTruncate::Overflow(TDes16 &/*aDes*/)
	{

	return;
	}
#else
void TestOverflowTruncate::Overflow(TDes8 &/*aDes*/)
	{

	return;
	}
#endif

EXPORT_C void PanicTFixedArray()
	{
	Panic(EBadFixedArrayIndex);
	}


// GCC support for X86
/*#if defined(__GCC32__) && defined(__X86__)
extern "C" 
{
EXPORT_C int __cxa_pure_virtual()
//
// Gets called for any unreplaced pure virtual methods.
//
	{
	Panic(EPureVirtualCalled);
	return 0;
	}


}
#endif*/
