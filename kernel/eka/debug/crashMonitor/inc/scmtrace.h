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
// scmdatatypes.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//


#ifndef __SCMTRACE_H_INCLUDED__
#define __SCMTRACE_H_INCLUDED__

#ifdef _DEBUG	
	#define __SCMFUNCTIONLOGGING __PRETTY_FUNCTION__ 
#else
	#define __SCMFUNCTIONLOGGING ""
#endif

// logger macro
#ifdef __KERNEL_MODE__

#include <kernel/kernel.h>
#include <nk_trace.h>

#define LOG_CONTEXT //__KTRACE_OPT(KALWAYS, Kern::Printf("Context --> <%s>", __SCMFUNCTIONLOGGING));
#define CLTRACE(s) __KTRACE_OPT(KDEBUGGER, Kern::Printf(s));
#define CLTRACE1(s, p1) __KTRACE_OPT(KDEBUGGER, Kern::Printf(s, p1));
#define CLTRACE2(s, p1, p2)  __KTRACE_OPT(KDEBUGGER, Kern::Printf(s, p1, p2));
#define CLTRACE3(s, p1, p2, p3) __KTRACE_OPT(KDEBUGGER, Kern::Printf(s, p1, p2, p3));
#define CLTRACE4(s, p1, p2, p3, p4) __KTRACE_OPT(KDEBUGGER, Kern::Printf(s, p1, p2, p3, p4));
#define CLTRACE5(s, p1, p2, p3, p4, p5) __KTRACE_OPT(KDEBUGGER, Kern::Printf(s, p1, p2, p3, p4, p5));

#else

#include <e32debug.h>

#define LOG_CONTEXT RDebug::Printf("Context --> <%s>", __SCMFUNCTIONLOGGING);
#define CLTRACE(s)  RDebug::Printf(s) ;
#define CLTRACE1(s, p1) RDebug::Printf(s, p1) ;
#define CLTRACE2(s, p1, p2) RDebug::Printf(s, p1, p2) ;
#define CLTRACE3(s, p1, p2, p3) RDebug::Printf(s, p1, p2, p3);
#define CLTRACE4(s, p1, p2, p3, p4) RDebug::Printf(s, p1, p2, p3, p4);
#define CLTRACE5(s, p1, p2, p3, p4, p5) RDebug::Printf(s, p1, p2, p3, p4, p5);


#endif


#endif /*__SCMTRACE_H_INCLUDED__*/
