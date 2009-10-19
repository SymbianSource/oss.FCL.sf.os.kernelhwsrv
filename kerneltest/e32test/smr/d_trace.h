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
// Bootstrap Shadow Memory Region Tests
//

#ifndef SMR_DEBUG_H
#define SMR_DEBUG_H

#include <e32err.h>
#include <e32const.h>
#include <e32def.h>
#include <e32cmn.h>
#include <e32des8.h>
#include <kernel/kernel.h>


// Make sure release builds get a warning if 
//#ifndef _DEBUG
//#if (defined SMR_TRACE)
//#warning "Use of Kern::PrintF tracing in a release build, check MMP files"
//#endif
//#endif

// Panic category string for SMR component
_LIT(KSMRPanicCategory, "T_SMR");


//
// MACROs for trace statements in client/server code.
//

#ifdef SMR_TRACE

#define SMR_LOG0(_text)		Kern::Printf((_text))
#define SMR_LOG1(_text, _a1)	Kern::Printf((_text), (_a1))
#define SMR_LOG_RETURN(_r1)	{ Kern::Printf("!-- Function exit : %d", (_r1)); \
                                return (_r1); } 
#define SMR_LOGMSG_RETURN(_s1, _r1)	{ Kern::Printf("!-- "_s1" (%d)", (_r1)); \
                                return (_r1); } 

#define SMR_TRACE0(_text)	Kern::Printf((_text))
#define SMR_TRACE1(_text, _a1)	Kern::Printf((_text), (_a1))
#define SMR_TRACE2(_text, _a1, _a2)	Kern::Printf((_text), (_a1), (_a2))
#define SMR_TRACE3(_text, _a1, _a2, _a3)	Kern::Printf((_text), (_a1), (_a2), (_a3))
#define SMR_TRACE4(_text, _a1, _a2, _a3, _a4)	Kern::Printf((_text), (_a1), (_a2), (_a3), (_a4))
#define SMR_TRACE5(_text, _a1, _a2, _a3, _a4, _a5)	Kern::Printf((_text), (_a1), (_a2), (_a3), (_a4), (_a5))
#define SMR_TRACE6(_text, _a1, _a2, _a3, _a4, _a5, _a6)	Kern::Printf((_text), (_a1), (_a2), (_a3), (_a4), (_a5), (_a6))

#define SMR_FUNC(_text)     TEntryExit _entryexit(_text)
#define SMR_FUNCE(_text)	 Kern::Printf("--> "##_text)
#define SMR_FUNCR(_text)	 Kern::Printf("<-- "##_text) 
#define SMR_FUNCR_return(_text, _r1)	 { Kern::Printf("<-- "##_text##"\n"); \
                                            return (_r1); }

#else

#define SMR_LOG0(_text)		Kern::Printf(_L(_text))
#define SMR_LOG1(_text, _a1)	Kern::Printf(_L(_text), (_a1))
#define SMR_LOG_RETURN(_r1)	    { return (_r1); } 
#define SMR_LOGMSG_RETURN(_s1, _r1)	{ return (_r1); } 


#define SMR_TRACE0(_text)			
#define SMR_TRACE1(_text, _a1)		
#define SMR_TRACE2(_text, _a1, _a2)
#define SMR_TRACE3(_text, _a1, _a2, _a3)	
#define SMR_TRACE4(_text, _a1, _a2, _a3, _a4)
#define SMR_TRACE5(_text, _a1, _a2, _a3, _a4, _a5)
#define SMR_TRACE6(_text, _a1, _a2, _a3, _a4, _a5, _a6)

#define SMR_FUNC(_text)   
#define SMR_FUNCE(_text)	 
#define SMR_FUNCR(_text)
#define SMR_FUNCR_return(_text, _r1)	 { return (_r1); }

#endif


class TEntryExit
{
public:
    inline TEntryExit(const char *aFn);
    inline ~TEntryExit();

public:
    const char* iName;
};

TEntryExit::TEntryExit(const char* aFn)
 : iName(aFn)
    {
    SMR_TRACE1("--> %s " , iName);
    };

TEntryExit::~TEntryExit()
    {
    SMR_TRACE1("<-- %s " , iName);
    };
    

#endif // SMR_DEBUG_H

