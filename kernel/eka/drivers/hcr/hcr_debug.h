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
// Hardware Configuration Repository Platform Independent Layer (PIL)
//

#ifndef HCR_DEBUG_H
#define HCR_DEBUG_H

#include <e32err.h>
#include <e32const.h>
#include <e32def.h>
#include <e32cmn.h>
#include <e32des8.h>
#include <kernel/kernel.h>


// Make sure release builds get a warning if 
//#ifndef _DEBUG
//#if (defined HCR_TRACE)
//#warning "Use of Kern::PrintF tracing in a release build, check MMP files"
//#endif
//#endif


//
// MACROs for trace statements in client/server code.
//

#ifdef _DEBUG

#define HCR_LOG0(_text)				Kern::Printf("=== "_text)
#define HCR_LOG1(_text, _a1)		Kern::Printf("... "_text, (_a1))
#define HCR_LOG_RETURN(_r1)			return (Kern::Printf("!-- HCR Function exit, error=%d (%s:%d)", (_r1), __FILE__, __LINE__), _r1)
#define HCR_LOGMSG_RETURN(_s1, _r1)	return (Kern::Printf("!-- HCR: "_s1" (%d)", (_r1)), _r1) 

#else

#define HCR_LOG0(_text)				
#define HCR_LOG1(_text, _a1)		
#define HCR_LOG_RETURN(_r1)			return (_r1)
#define HCR_LOGMSG_RETURN(_s1, _r1)	return (_r1) 

#endif



#ifdef HCR_TRACE

#define HCR_TRACE0(_text)								Kern::Printf((_text))
#define HCR_TRACE1(_text, _a1)							Kern::Printf((_text), (_a1))
#define HCR_TRACE2(_text, _a1, _a2)						Kern::Printf((_text), (_a1), (_a2))
#define HCR_TRACE3(_text, _a1, _a2, _a3)				Kern::Printf((_text), (_a1), (_a2), (_a3))
#define HCR_TRACE4(_text, _a1, _a2, _a3, _a4)			Kern::Printf((_text), (_a1), (_a2), (_a3), (_a4))
#define HCR_TRACE5(_text, _a1, _a2, _a3, _a4, _a5)		Kern::Printf((_text), (_a1), (_a2), (_a3), (_a4), (_a5))
#define HCR_TRACE6(_text, _a1, _a2, _a3, _a4, _a5, _a6)	Kern::Printf((_text), (_a1), (_a2), (_a3), (_a4), (_a5), (_a6))
#define HCR_TRACE_RETURN(_r1)	 						return (Kern::Printf("!-- Function exit return(%d) (%s:%d)", (_r1), __FILE__, __LINE__), _r1)
#define HCR_TRACEMSG_RETURN(_s1, _r1)	 				return (Kern::Printf("!-- "_s1" (%d)", (_r1)), _r1)
#define HCR_FUNC(_text)     							TEntryExit _entryexit(_text)

#define HCR_HEX_DUMP_ABS(_address, _length)	HexDump((_address), (_length));			//Hex dump with absolute address
#define HCR_HEX_DUMP_REL(_address, _length)	HexDump((_address), (_length), EFalse); //Hex dump with relative (from) address


#else

#define HCR_TRACE0(_text)			
#define HCR_TRACE1(_text, _a1)		
#define HCR_TRACE2(_text, _a1, _a2)
#define HCR_TRACE3(_text, _a1, _a2, _a3)	
#define HCR_TRACE4(_text, _a1, _a2, _a3, _a4)
#define HCR_TRACE5(_text, _a1, _a2, _a3, _a4, _a5)
#define HCR_TRACE6(_text, _a1, _a2, _a3, _a4, _a5, _a6)
#define HCR_TRACE_RETURN(_r1)		return (_r1)
#define HCR_TRACEMSG_RETURN(_r1)	return (_r1)
#define HCR_FUNC(_text)   
#define HCR_HEX_DUMP_ABS(_address, _length)
#define HCR_HEX_DUMP_REL(_address, _length)

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
    HCR_TRACE1("--> %s " , iName);
    };

TEntryExit::~TEntryExit()
    {
    HCR_TRACE1("<-- %s " , iName);
    };
    

void HexDump(TUint8* aStartAddress, TUint32 aLength, TBool aAbsolute = ETrue);


#endif // HCR_DEBUG_H

