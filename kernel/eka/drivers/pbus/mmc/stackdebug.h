// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
*/

#ifndef DEBUG_H
#define DEBUG_H

#define _STACK_DEBUG_PRINT_ 

#if defined(_STACK_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string  */
#define __PRINT(t) {Kern::Printf(t);}
/** Trace - format string with 1 param */
#define __PRINT1(t,a) {Kern::Printf(t,a);}
/** Trace - format string with 2 params */
#define __PRINT2(t,a,b) {Kern::Printf(t,a,b);}
/** Trace - format string with 3 params */
#define __PRINT3(t,a,b,c) {Kern::Printf(t,a,b,c);}
/** Trace - format string with 4 params */
#define __PRINT4(t,a,b,c,d) {Kern::Printf(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __PRINT5(t,a,b,c,d,e) {Kern::Printf(t,a,b,c,d,e);}

#else

/** NULL definition */
#define __PRINT(t)
/** NULL definition */
#define __PRINT1(t,a)
/** NULL definition */
#define __PRINT2(t,a,b)
/** NULL definition */
#define __PRINT3(t,a,b,c)
/** NULL definition */
#define __PRINT4(t,a,b,c,d)
/** NULL definition */
#define __PRINT5(t,a,b,c,d,e)
#endif

#endif // DEBUG_H
