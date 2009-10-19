// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

//#define _SCSI_DEBUG_PRINT_
//#define _BOT_DEBUG_PRINT_
//#define _HOST_DEBUG_PRINT_
//#define _TESTREPORT_PRINT_

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#include <e32debug.h>
#endif

#if defined(_SCSI_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string  */
#define __SCSIPRINT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __SCSIPRINT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __SCSIPRINT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __SCSIPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __SCSIPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __SCSIPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
/** NULL definition */
#define __SCSIPRINT(t)
/** NULL definition */
#define __SCSIPRINT1(t,a)
/** NULL definition */
#define __SCSIPRINT2(t,a,b)
/** NULL definition */
#define __SCSIPRINT3(t,a,b,c)
/** NULL definition */
#define __SCSIPRINT4(t,a,b,c,d)
/** NULL definition */
#define __SCSIPRINT5(t,a,b,c,d,e)
#endif


#if defined(_BOT_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string */
#define __BOTPRINT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __BOTPRINT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __BOTPRINT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __BOTPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __BOTPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __BOTPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
/** NULL definition */
#define __BOTPRINT(t)
/** NULL definition */
#define __BOTPRINT1(t,a)
/** NULL definition */
#define __BOTPRINT2(t,a,b)
/** NULL definition */
#define __BOTPRINT3(t,a,b,c)
/** NULL definition */
#define __BOTPRINT4(t,a,b,c,d)
/** NULL definition */
#define __BOTPRINT5(t,a,b,c,d,e)
#endif


#if defined(_HOST_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string */
#define __HOSTPRINT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __HOSTPRINT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __HOSTPRINT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __HOSTPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __HOSTPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __HOSTPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
/** NULL definition */
#define __HOSTPRINT(t)
/** NULL definition */
#define __HOSTPRINT1(t,a)
/** NULL definition */
#define __HOSTPRINT2(t,a,b)
/** NULL definition */
#define __HOSTPRINT3(t,a,b,c)
/** NULL definition */
#define __HOSTPRINT4(t,a,b,c,d)
/** NULL definition */
#define __HOSTPRINT5(t,a,b,c,d,e)
#endif


#if defined(_TESTREPORT_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
/** Trace - format string */
#define __TESTREPORT(t) {RDebug::Print(t);}
/** Trace - format string with 1 param */
#define __TESTREPORT1(t,a) {RDebug::Print(t,a);}
/** Trace - format string with 2 params */
#define __TESTREPORT2(t,a,b) {RDebug::Print(t,a,b);}
/** Trace - format string with 3 params */
#define __TESTREPORT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
/** Trace - format string with 4 params */
#define __TESTREPORT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
/** Trace - format string with 5 params */
#define __TESTREPORT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
/** NULL definition */
#define __TESTREPORT(t)
/** NULL definition */
#define __TESTREPORT1(t,a)
/** NULL definition */
#define __TESTREPORT2(t,a,b)
/** NULL definition */
#define __TESTREPORT3(t,a,b,c)
/** NULL definition */
#define __TESTREPORT4(t,a,b,c,d)
/** NULL definition */
#define __TESTREPORT5(t,a,b,c,d,e)
#endif

#endif // DEBUG_H
