// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Logging macros for use in debug subsystem
// 
//

#ifndef DEBUG_LOGGING_H
#define DEBUG_LOGGING_H

/**
 * Debug messages
 * 
 * Debug messages are only generated for debug builds.
 * 
 * For kernel mode, use __KTRACE_OPT(KDEBUGGER, Kern::Printf(), 
 * for user mode use RDebug::Printf(). 
 * 
 */

#ifdef _DEBUG

  #ifdef __KERNEL_MODE__

	#include <kernel/kernel.h>
	#include <nk_trace.h>

	#define LOG_MSG( a )				__KTRACE_OPT(KDEBUGGER, Kern::Printf( a ))
	#define LOG_MSG2( a, b )			__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b ))
	#define LOG_MSG3( a, b, c )			__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b, c ))
	#define LOG_MSG4( a, b, c, d )		__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b, c, d ))
	#define LOG_MSG5( a, b, c, d, e )	__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b, c, d, e ))

	#ifdef __LOG_EVENTS__

	#define LOG_EVENT_MSG( a )				__KTRACE_OPT(KDEBUGGER, Kern::Printf( a ))
	#define LOG_EVENT_MSG2( a, b )			__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b ))
	#define LOG_EVENT_MSG3( a, b, c )		__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b, c ))
	#define LOG_EVENT_MSG4( a, b, c, d )	__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b, c, d ))
	#define LOG_EVENT_MSG5( a, b, c, d, e )	__KTRACE_OPT(KDEBUGGER, Kern::Printf( a, b, c, d, e ))
	
	#else

	#define LOG_EVENT_MSG( a )
	#define LOG_EVENT_MSG2( a, b )
	#define LOG_EVENT_MSG3( a, b, c )
	#define LOG_EVENT_MSG4( a, b, c, d )
	#define LOG_EVENT_MSG5( a, b, c, d, e )

	#endif

  #else

    #include <e32debug.h>

	#define LOG_MSG( a )				RDebug::Printf( a )
	#define LOG_MSG2( a, b )			RDebug::Printf( a, b )
	#define LOG_MSG3( a, b, c )			RDebug::Printf( a, b, c )
	#define LOG_MSG4( a, b, c, d )		RDebug::Printf( a, b, c, d )
	#define LOG_MSG5( a, b, c, d, e )	RDebug::Printf( a, b, c, d, e )

	#ifdef __LOG_EVENTS__

	#define LOG_EVENT_MSG( a )					RDebug::Printf( a )
	#define LOG_EVENT_MSG2( a, b )				RDebug::Printf( a, b )
	#define LOG_EVENT_MSG3( a, b, c )			RDebug::Printf( a, b, c )
	#define LOG_EVENT_MSG4( a, b, c, d )		RDebug::Printf( a, b, c, d )
	#define LOG_EVENT_MSG5( a, b, c, d, e )		RDebug::Printf( a, b, c, d, e )

	#else

	#define LOG_EVENT_MSG( a )
	#define LOG_EVENT_MSG2( a, b )
	#define LOG_EVENT_MSG3( a, b, c )
	#define LOG_EVENT_MSG4( a, b, c, d )
	#define LOG_EVENT_MSG5( a, b, c, d, e )

    #endif

    #endif
#else

	#define LOG_MSG( a )
	#define LOG_MSG2( a, b )
	#define LOG_MSG3( a, b, c )
	#define LOG_MSG4( a, b, c, d )
	#define LOG_MSG5( a, b, c, d, e )

	#define LOG_EVENT_MSG( a )
	#define LOG_EVENT_MSG2( a, b )
	#define LOG_EVENT_MSG3( a, b, c )
	#define LOG_EVENT_MSG4( a, b, c, d )
	#define LOG_EVENT_MSG5( a, b, c, d, e )

#endif

#endif //DEBUG_LOGGING_H

