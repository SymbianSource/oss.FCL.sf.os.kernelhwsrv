// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef RMDEBUG_MULTI_AGENT_LOGGING_H
#define RMDEBUG_MULTI_AGENT_LOGGING_H

/* Debug messages
 * 
 * Debug messages are only generated for debug builds.
 * 
 * As user mode use RDebug::Printf(). 
 * 
 */

// Uncomment if logging of multi agent test required
// #define MULTI_AGENT_DEBUG_LOGGING 

#ifdef MULTI_AGENT_DEBUG_LOGGING

    #include <e32debug.h>

    #define LOG_MSG( a )              RDebug::Printf( a )
    #define LOG_MSG2( a, b )          RDebug::Printf( a, b )
    #define LOG_MSG3( a, b, c )       RDebug::Printf( a, b, c )
    #define LOG_MSG4( a, b, c, d )    RDebug::Printf( a, b, c, d )
    #define LOG_MSG5( a, b, c, d, e )    RDebug::Printf( a, b, c, d, e )

#else

  #define LOG_MSG( a )
  #define LOG_MSG2( a, b )
  #define LOG_MSG3( a, b, c )
  #define LOG_MSG4( a, b, c, d )
  #define LOG_MSG5( a, b, c, d, e )

#endif

#endif //RMDEBUG_MULTI_AGENT_LOGGING_H
