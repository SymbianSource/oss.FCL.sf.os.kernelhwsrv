// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\compsupp\rvct2_2\rvct2_2.h
// This is the preinclude file for the rvct 2.2 compiler
// It contains all the compiler specific definitions required by the SOS source
// 
//

/**
 @file
 @publishedAll
 @released
*/

#ifdef __ARMCC_VERSION
#if (__ARMCC_VERSION < 220435 || __ARMCC_VERSION >= 230000)
#error This instantiation of the build requires use of RVCT 2.2 Build 435 (or later)
#endif
#endif

#include <rvct/rvct.h>

