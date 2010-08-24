// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @internalComponent
// Collection of utility functions primarily assisting debuggers and maintainers.
// 
//

#ifndef DEBUGMACROS_H
#define DEBUGMACROS_H

#include <e32debug.h> // RDebug class
#include "testengine.h"

/**
Debug macro for serial port logging of function names and signatures
 e.g. 
<Function> void CMyClass::MyFunction (TInt aParam)
*/

const TInt KErrAssertionInternal	= 3;

#define TEST_ASSERTION(a,b) 	if (!(a)) {test.Printf(_L("ASSERTION FAILED!\n line %d file %S.\n"), \
									__LINE__, __FILE__); test.Printf(b); \
									__ASSERT_ALWAYS(a, User::Panic(_L("Test F/W Err"), KErrAssertionInternal));};



/**
Debug macro to output test in 'verbose' detail mode
*/
#define LOG_VERBOSE1(a) if (gVerboseOutput) test.Printf(a);
#define LOG_VERBOSE2(a,b) if (gVerboseOutput) test.Printf(a, b);
#define LOG_VERBOSE3(a,b,c) if (gVerboseOutput) test.Printf(a, b, c);

#define LOG_STEPNAME(a) PrintStepName(a); 

/**
 Panic the framework
*
#define PANIC_FRAMEWORK(a) RDebug::Printf("<Framework died '%s' %d %s",\
    a, __LINE__, __FILE__); User::Panic(_LIT("Test F/W"), KErrUnknown);
*/	


#endif // DEBUGMACROS_H

