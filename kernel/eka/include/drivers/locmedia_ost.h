// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\locmedia_ost.h
// 
//

#ifndef LOCMEDIA_OST_H
#define LOCMEDIA_OST_H

#ifndef _DEBUG

#ifdef OstTrace0
#undef OstTrace0
#define OstTrace0( aGroupName, aTraceName, aTraceText )
#endif

#ifdef OstTrace1
#undef OstTrace1
#define OstTrace1( aGroupName, aTraceName, aTraceText, aParam )
#endif

#ifdef OstTraceData
#undef OstTraceData
#define OstTraceData( aGroupName, aTraceName, aTraceText, aPtr, aLength )
#endif

#ifdef OstTraceExt1
#undef OstTraceExt1
#define OstTraceExt1( aGroupName, aTraceName, aTraceText, aParam )
#endif

#ifdef OstTraceExt2
#undef OstTraceExt2
#define OstTraceExt2( aGroupName, aTraceName, aTraceText, aParam1, aParam2 )
#endif

#ifdef OstTraceExt3
#undef OstTraceExt3
#define OstTraceExt3( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3 )
#endif

#ifdef OstTraceExt4
#undef OstTraceExt4
#define OstTraceExt4( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4 )
#endif

#ifdef OstTraceExt5
#undef OstTraceExt5
#define OstTraceExt5( aGroupName, aTraceName, aTraceText, aParam1, aParam2, aParam3, aParam4, aParam5 )
#endif

#ifdef OstTraceFunctionEntry0
#undef OstTraceFunctionEntry0
#define OstTraceFunctionEntry0( aTraceName )
#endif

#ifdef OstTraceFunctionEntry1
#undef OstTraceFunctionEntry1
#define OstTraceFunctionEntry1( aTraceName, aInstance )
#endif

#ifdef OstTraceFunctionEntryExt
#undef OstTraceFunctionEntryExt
#define OstTraceFunctionEntryExt(aTraceName, aInstance)
#endif

#ifdef OstTraceFunctionExit0
#undef OstTraceFunctionExit0
#define OstTraceFunctionExit0( aTraceName )
#endif

#ifdef OstTraceFunctionExit1
#undef OstTraceFunctionExit1
#define OstTraceFunctionExit1( aTraceName, aInstance )
#endif

#ifdef OstTraceEventStart0
#undef OstTraceEventStart0
#define OstTraceEventStart0( aTraceName, aEventName )
#endif

#ifdef OstTraceEventStart1
#undef OstTraceEventStart1
#define OstTraceEventStart1( aTraceName, aEventName, aParam )
#endif

#ifdef OstTraceFunctionExitExt
#undef OstTraceFunctionExitExt
#define OstTraceFunctionExitExt(aTraceName, aInstance, aRetval)
#endif

#ifdef OstTraceEventStop
#undef OstTraceEventStop
#define OstTraceEventStop( aTraceName, aEventName, aStartTraceName )
#endif

#ifdef OstTraceState0
#undef OstTraceState0
#define OstTraceState0( aTraceName, aStateName, aNewState )
#endif

#ifdef OstTraceState1
#undef OstTraceState1
#define OstTraceState1( aTraceName, aStateName, aNewState, aInstance )
#endif

#endif // _DEBUG

#endif // LOCMEDIA_OST_H
